/**************************************************************************
 *                                  
 *   ==== ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====                              
 *   ==== ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====                                                    
 *   ==== ==== ===== ===== ===== ===== ===== ===== ===== ===== ===== =====                                 
 * 
 *                     ( (_) )                  ( (_) )
 *                                    
 *                                     ^
 *                                    
 *                              (-------------)        
 *                                    
 *                    <<<<<<<< A I R D U I N O >>>>>>>
 *    <<<<<<<< Copyright Ivan Cerra De Castro - Marzo 2021 >>>>>>>>>>>
 *           <<<<<<<< https://www.ivancerra.com >>>>>>>>>>>
 * 
 *                 Video: https://youtu.be/
 * ************************************************************************
 * Emulacion de BreakOut para Arduino 
 * 
 * Componentes usados y conexiones:
 * 1. Arduino UNO.
 * 2. OLED AZDelivery SH 1106 I2C 128x64
 *    Conexiones: 
 *        · SCK -> A5
 *        · SDA -> A4
 *        · GND -> GND
 *        · VCC -> 5V
 * 3. CCS811 
 *    Conexiones:
 *        · SCK -> A5
 *        · SDA -> A4
 *        · GND -> GND
 *        · VCC -> 3.3V
 *        · WAK -> GND
 * 4. DHT22
 *    Conexiones:
 *        · PIN -> D5
 **************************************************************************
 *
 * La concentración de CO2 al aire libre oscila entre 360 ppm 
 * (parts per million) en áreas de aire limpio y 700 ppm en las ciudades. 
 * El valor máximo recomendado para los interiores es de 1.000 ppm 
 * y el valor límite para oficinas es de 1.500 ppm
 * Confort < 1000 ppm
 * Desagradable 1000 - 5000 ppm
 * Muy desagradable 5000 - 30000 ppm
 * Toxico > 30000 ppm
 * 
 * TVOC, es el acrónimo para el total de compuestos orgánicos volátiles.
 * Confort < 200 ppb
 * Desagradable de 200 - 3000 ppb
 * Muy desagradable 3000 - 25000 ppb
 * Toxico > 25000 ppb
 * 
 **************************************************************************/

// Libreria para pantallas soporte para controladoras SPI y I2C:
// SSD1325, ST7565, ST7920, UC1608, UC1610, UC1701, PCD8544, PCF8812, KS0108, 
// LC7981, SBN1661, SSD1306, SH1106, T6963, LD7032
#include <U8glib.h>

// Libreria para sensores DHT11 y DHT 22
#include <DHT.h>

// Libreria para Sensores de CO2 y TVOC CCS811/CJMCU811
#include <Adafruit_CCS811.h>

/*************************************************
// Direcciones de los Sensores I2C Del Proyecto
// Address I2C CCS811 -> 90
// Address I2C OLED SH1106 -> 60
**************************************************/

// Para sensor de Temperatura y Humedad
#define DHTTYPE DHT22
#define DHT_PIN 5

// Puntos de muestreo para el histograma
#define MAX_HISTOGRAM 40

// Valor minimo, maximo en la Y del histograma
#define MAX_CO2_HISTOGRAM 1000
#define MIN_CO2_HISTOGRAM 400

// Tiempo de bucle en ms
#define TIME_LOOP 1000

// Tiempo que esta Visible CO2 y TVOC Medidos en Tiempo de Bucle
#define TIME_CO2 8
#define TIME_TVOC 2

// Objeto para control de OLED SH1106
U8GLIB_SH1106_128X64 _u8g(U8G_I2C_OPT_NONE);

// Flag que indica si muestra CO2 o TVOC
uint8_t _co2VSTVOC;

// Estructura de punto
struct Point
{
  uint8_t x;
  uint8_t y;

  /**************************************************************************
    Constructor
  **************************************************************************/
  Point():x(0), y(0) {}
  Point(uint8_t px, uint8_t py):x(px), y(py) {}
};

// Estructura de tamaño
struct Size
{
  uint8_t width;
  uint8_t height;

  /**************************************************************************
    Constructor
  **************************************************************************/
  Size(): width(0), height(0) {}
  Size(uint8_t pWidth, uint8_t pHeight):width(pWidth), height(pHeight) {}
};

// Estructura de ventana
struct Window
{
  Point origin;
  Size size;

  /**************************************************************************
    Constructor
  **************************************************************************/
  Window():origin(0, 0), size(0, 0) {}
  Window(Point pOrigin, Size pSize):origin(pOrigin.x, pOrigin.y), size(pSize.width, pSize.height) {}
  Window(uint8_t px, uint8_t py, uint8_t pWidth, uint8_t pHeight):origin(px, py), size(pWidth, pHeight) {}
};

/**************************************************************************
/**************************************************************************
//  E S T R U C T U R A    H I S T O G R A M A
/**************************************************************************
/**************************************************************************/
struct Histogram
{
  // Array de histograma 
  uint16_t arrHistogram[MAX_HISTOGRAM];

  // Paso de muestreo
  uint16_t histogramStep;
  // Cada cuantos Pasos de muestreo volcamos la media al array
  uint16_t histogramStepMax;
  
  // Media de muestreo para ese paso
  float average;

  /**************************************************************************
    Constructor
  **************************************************************************/
  Histogram(uint16_t stepMax = 1):histogramStep(0), average(0), histogramStepMax((stepMax == 0)?1:stepMax) 
  {
    // Inicialización de Histograma
    for(int i = 0; i < MAX_HISTOGRAM; i++)
    {
      arrHistogram[i] = 0;
    }
  }

  /**************************************************************************
    Comprobamos si añadimos dato al Histograma
  **************************************************************************/
  void addValue(float value)
  {
    if(histogramStep == 0)
    {
      average = value;
    }
    else
    {
      average = (average + value) / 2.0;
    }

    histogramStep++;

    if(histogramStep == histogramStepMax)
    {
      addHistogram(average);
      average = 0;
      histogramStep = 0;
    }
  }

  /**************************************************************************
    Añadimos dato al Histograma
  **************************************************************************/
  void addHistogram(float value)
  {
    // Desplazamos el histograma completo
    for(int i = MAX_HISTOGRAM - 1; i > 0; i--)
    {
      arrHistogram[i] = arrHistogram[i - 1];
    }

    // metemos el nuevo valor
    arrHistogram[0] = (uint16_t)value;
  }

  /**************************************************************************
    Recuperamos un valor del histograma
  **************************************************************************/
  uint16_t getValue(uint8_t index)
  {
    if(index < 0 || index >= MAX_HISTOGRAM)
    {
      return 0;
    }

    return arrHistogram[index];
  }
};

/**************************************************************************
/**************************************************************************
//  E S T R U C T U R A    S E N S O R E S
/**************************************************************************
/**************************************************************************/
struct Sensors
{
  // Objeto para control de sensor de CO2
  Adafruit_CCS811 ccs;

  // Objeto para control sensor Humedad y temperatura
  DHT dht = DHT(DHT_PIN, DHTTYPE);

  // Buffers de cadena donde ponemos los datos de los sensores, formateados
  char bufTemperature[10];
  char bufHumidity[10];
  char bufCO2[10];
  char bufTVOC[10];

  // Valores de los sensores
  float cO2;
  float tVOC;
  float temperature;
  float humidity;

  /**************************************************************************
    Constructor
  **************************************************************************/
  Sensors():cO2(0), tVOC(0), temperature(0), humidity(0) {}

  /**************************************************************************
    Inicializacion
  **************************************************************************/
  void setupSensors()
  {
    // Iniciamos sensor temp y humedad
    dht.begin();
 
    // Iniciamos el sensor de CO2
    ccs.begin();
 
    // Esperamos que el sensor este listo
    while(!ccs.available());

    // lecturas cada 250ms
    ccs.setDriveMode(CCS811_DRIVE_MODE_250MS);
  }

  /**************************************************************************
    Leemos los datos de los sensores
  **************************************************************************/
  void readSensorsValues()
  {
    // Leemos humedad y temperatura
    humidity = dht.readHumidity();
    
    temperature = dht.readTemperature();
    
    // Leemos CO2 y TVOC
    if(!ccs.readData())
    {
      cO2 = ccs.geteCO2();

      tVOC= ccs.getTVOC();
    }    
  }

  /**************************************************************************
    Convertimos el Dato a cadena
  **************************************************************************/
  void dataIntToStr(float data, char *str, char unity)
  {
    // Parte entera
    int iData = (int)data;
  
    sprintf(str, "%4d%c", iData, unity);
  }

  /**************************************************************************
    Convertimos el Dato a cadena
  **************************************************************************/
  void dataFloatToStr(float data, char *str, char unity)
  {
    // Parte entera
    int iData = (int)data;
    // Parte decimal
    int dData = (int)(data * 10) % 10;

    sprintf(str, "%5d.%d%c", iData, dData, unity);
  }

  /**************************************************************************
    Devuelve la temperatura en cadena
  **************************************************************************/
  const char * strTemperature()
  {
    // Signo de grado es 176 o 0xB0  ->  ° 
    dataFloatToStr(temperature, bufTemperature, 176);

    return bufTemperature;
  }

  /**************************************************************************
    Devuelve la Humedad en cadena
  **************************************************************************/
  const char * strHumidity()
  {
    dataFloatToStr(humidity, bufHumidity, '%');

    return bufHumidity;
  }

  /**************************************************************************
    Devuelve el CO2 en cadena
  **************************************************************************/
  const char * strCO2()
  {
    dataIntToStr(cO2, bufCO2, ' ');

    return bufCO2;
  }

  /**************************************************************************
    Devuelve el TVOC en cadena
  **************************************************************************/
  const char * strTVOC()
  {
    dataIntToStr(tVOC, bufTVOC, ' ');

    return bufTVOC;
  }
};

// Ventanas donde mostramos los datos y el histograma
Window _winData;
Window _winHistogram;

// Objeto de Histograma
Histogram _histogramCO2 = Histogram(60);

// Objeto de Sensores
Sensors _sensors;

// Tamaño de pantalla
Size _screenSize;
 
/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   I N I C I A L I Z A C I O N
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Rutinas de inicializacion
 **************************************************************************/
void setup() 
{
  // Abrimos el puerto Serie
  Serial.begin(115200);

  _sensors.setupSensors();

  // Conmutador de visionado de CO2 y TVOC
  _co2VSTVOC = 0;

  // Dimensiones de la pantalla
  _screenSize = Size(_u8g.getWidth(), _u8g.getHeight());
  
  // Definimos ventana que mostrara los datos
  _winData = Window(0, _screenSize.height / 2, _screenSize.width, _screenSize.height / 2);
  
  // Definimos la ventana que mostrara el historial
  _winHistogram = Window(0, 0, _screenSize.width, _screenSize.height / 2 - 4);

  // Inicializacion de pantalla
  setupScreen();
}
 
/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E  P R O G R A M A   P R I N C I P A L
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Bucle principal
 **************************************************************************/
void loop() 
{
  // Leemos los datos de los sensores
  _sensors.readSensorsValues();

  // Añadimos valor de CO2
  _histogramCO2.addValue(_sensors.cO2);

  // Mostramos log por el Serie
  logSerial();

  // Importante no hacer cambios de Valores del programa en la rutina de pintado de pantalla
  // Hay un bucle de refresco, sino se solaparian 
  // Mostramos datos en pantalla
  drawScreen();

  // Para el cambio de CO2 y TVOC
  _co2VSTVOC = (_co2VSTVOC + 1) % (TIME_CO2 + TIME_TVOC);

  // Tiempo de refresco
  delay(TIME_LOOP);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   L O G S 
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Mostramos log por perto Serie
 **************************************************************************/
void logSerial()
{
  Serial.print("Temp: ");
  Serial.print(_sensors.strTemperature());
  Serial.print(" Hum: ");
  Serial.print(_sensors.strHumidity());
  Serial.print(" CO2: ");
  Serial.print(_sensors.strCO2());
  Serial.print("ppm AV CO2: ");
  Serial.print(_histogramCO2.average);
  Serial.print("ppm, TVOC: ");
  Serial.print(_sensors.strTVOC());
  Serial.println("ppb");
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   I N T E R F A Z
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos la pantalla
 **************************************************************************/
void drawScreen()
{
  // Refresca la pantalla por Paginas, importante no cambiar datos en medio de este refresco
  // Se solaparian los graficos
  _u8g.firstPage();  
  do 
  {
    // Pintamos ventana de Datos
    drawFrame(_winData);
    // Pintamos los datos
    drawData(_winData);
    
    // Pintamos los ejes del histograma
    drawHistogramAxis(_winHistogram);
    // Pintamos el histograma
    drawHistogram(_winHistogram);
  } while(_u8g.nextPage());
}

/**************************************************************************
  Pintamos el frame
 **************************************************************************/
void drawFrame(Window win)
{
  // Cuadricula donde metemos los datos
  drawRect(win.origin.x, win.origin.y, win.size.width, win.size.height);

  drawLine(win.origin.x + win.size.width / 2, win.origin.y, win.origin.x + win.size.width / 2, win.origin.y + win.size.height);

  drawLine(win.origin.x + win.size.width / 2,  win.origin.y + win.size.height / 2, win.origin.x + win.size.width, win.origin.y + win.size.height / 2);
}

/**************************************************************************
  Pintamos los datos
 **************************************************************************/
void drawData(Window win)
{
  drawText(win.origin.x + win.size.width / 2 + 2,  win.origin.y + 4, _sensors.strTemperature());

  drawText(win.origin.x + win.size.width / 2 + 2,  win.origin.y + win.size.height / 2 + 3, _sensors.strHumidity());

  // Alternamos la visualizacion de TVOC Y CO2
  if(_co2VSTVOC < TIME_CO2)
  {
    drawText(win.origin.x + 12,  win.origin.y + 2, "CO2 ppm");
    drawTextLarge(win.origin.x + 10,  win.origin.y + 12, _sensors.strCO2());
  }
  else
  {
    drawText(win.origin.x + 10,  win.origin.y + 2, "TVOC ppb");
    drawTextLarge(win.origin.x + 10, win.origin.y + 12, _sensors.strTVOC());
  }
}

/**************************************************************************
  Pintamos el Eje del histograma
 **************************************************************************/
void drawHistogramAxis(Window win)
{
  // Ejes del histograma
  drawLine(win.origin.x + 12, win.origin.y, win.origin.x + 12, win.origin.y + win.size.height);
  drawLine(win.origin.x + 12, win.origin.y + win.size.height, win.origin.x + win.size.width, win.origin.y + win.size.height);

  // Etiqueta del eje, la pintamos en vertical
  drawTextV(win.origin.x, win.origin.y + win.size.height / 2 + 7, "CO2");
}

/**************************************************************************
  Quantificamos dato de CO2 para histograma
 **************************************************************************/
uint8_t quantumValueCO2(uint16_t co2Value, uint16_t height)
{
  // Normalizamos el valor
  float normalizedValue = min (MAX_CO2_HISTOGRAM , co2Value);

  normalizedValue = max(normalizedValue, MIN_CO2_HISTOGRAM);
  normalizedValue = (normalizedValue - MIN_CO2_HISTOGRAM) / (MAX_CO2_HISTOGRAM - MIN_CO2_HISTOGRAM);

  // Calculamos cuantos quantum de 3 pixels entran
  uint8_t quantum = height / 3;

  // Este es el numero de unidades quantificadas
  normalizedValue = normalizedValue * quantum;

  return (uint8_t) normalizedValue;
}

/**************************************************************************
  Pintamos el Histograma
 **************************************************************************/
void drawHistogram(Window win)
{
  for(int index = 0; index < MAX_HISTOGRAM; index++)
  {
    // Quantificamos el valor
    uint8_t quantumData = quantumValueCO2(_histogramCO2.getValue(index), win.size.height);

    for(int quanto = 0; quanto <= quantumData; quanto++)
    {
      drawRect(index * 3 + win.origin.x + 14, win.origin.y + win.size.height - 3 - (quanto * 3), 2, 2);
    }
  }
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   G R A F I C A S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Setup del OLED
 **************************************************************************/
void setupScreen(void) 
{
  _u8g.setFont(u8g_font_6x10);
  _u8g.setFontRefHeightExtendedText();
  _u8g.setDefaultForegroundColor();
  _u8g.setFontPosTop();
}

/**************************************************************************
  Pintamos Rectangulo
 **************************************************************************/
void drawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
  _u8g.drawFrame(x, y, width, height);
}

/**************************************************************************
  Pintamos Linea
 **************************************************************************/
void drawLine(uint8_t x, uint8_t y, uint8_t xf, uint8_t yf)
{
  _u8g.drawLine(x, y, xf, yf);
}

/**************************************************************************
  Pintamos Texto
 **************************************************************************/
void drawText(uint8_t x, uint8_t y, const char *str)
{
   _u8g.drawStr(x,  y, str);
}

/**************************************************************************
  Pintamos Texto En Vertical
 **************************************************************************/
void drawTextV(uint8_t x, uint8_t y, const char *str)
{
   _u8g.drawStr270(x,  y, str);
}

/**************************************************************************
  Pintamos Texto x2
 **************************************************************************/
void drawTextLarge(uint8_t x, uint8_t y, const char *str)
{
  _u8g.setScale2x2();
  _u8g.drawStr(x / 2,  y / 2, str);
  _u8g.undoScale();
}
