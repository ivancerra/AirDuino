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
 
 Monitorización de la calidad del Aire en estancias.
    
 Componentes usados y conexiones:
  1. Arduino NANO.
  2. OLED AZDelivery SH 1106 I2C 128x64
     Conexiones: 
         · SCK -> A5
         · SDA -> A4
         · GND -> GND
         · VCC -> 5V
  3. CCS811 
     Conexiones:
         · SCK -> A5
         · SDA -> A4
         · GND -> GND
         · VCC -> 3.3V
         · WAK -> GND
  4. DHT22
     Conexiones:
         · PIN -> D5
 
  La concentración de CO2 al aire libre oscila entre 360 ppm 
  (parts per million) en áreas de aire limpio y 700 ppm en las ciudades. 
  El valor máximo recomendado para los interiores es de 1.000 ppm 
  y el valor límite para oficinas es de 1.500 ppm
  Confort < 1000 ppm
  Desagradable 1000 - 5000 ppm
  Muy desagradable 5000 - 30000 ppm
  Toxico > 30000 ppm
  
  TVOC, es el acrónimo para el total de compuestos orgánicos volátiles.
  Confort < 200 ppb
  Desagradable de 200 - 3000 ppb
  Muy desagradable 3000 - 25000 ppb
  Toxico > 25000 ppb
  
************************************************************************

    <<<<<<<< Copyright Ivan Cerra De Castro - Marzo 2021 >>>>>>>>>>>
 
            <<<<<<<< https://www.ivancerra.com >>>>>>>>>>>
