//AS5055 READ DATA PROGRAM

#include <SPI.h>
#define LED_BUILTIN 2
#define AlarmHigh_b (1<<14) //bit 13
#define AlarmLow_b (1<<15)  //bit 14
#define ErrorFlag_b (1<1) 
#define DataMask 0x3FFF

#define HSPI_MISO   12
#define HSPI_MOSI   13
#define HSPI_SCLK   14
#define HSPI_SS     15

uint16_t result1 = 5;
uint16_t resultado_prev = 6;
uint16_t resultado = 5;
uint16_t init_pos = 1000;
uint8_t AH = 0;
uint8_t AL = 0;
uint8_t EF = 0;
uint8_t PAR = 0;
uint8_t error = 0;

uint16_t message = 0xFFFF;

void setup() {
  Serial.begin(115200);

  SPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
  SPI.setBitOrder(MSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV16);  //you can chose faster SPI frequency
  SPI.setDataMode(SPI_MODE1); // Very important CPOL:0 PHA:1

  Serial.println("-----------------");
  Serial.println(HSPI_MISO); //12
  Serial.println(HSPI_MOSI); //13
  Serial.println(HSPI_SCLK);  //14
  Serial.println(HSPI_SS); //15
  
  Serial.println("-----------------");
  pinMode(HSPI_SS, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  
  result1 = sendMessage16(message, HSPI_SS);
  init_pos = (result1 & DataMask)>>2;

  Serial.println("VVVVVVVVVV");
  Serial.println(result1);
  Serial.println(init_pos);
  Serial.println((float)(init_pos)/11.38f);
  Serial.println("^^^^^^^^^^\n");
}

void loop() {
  float resp;
  result1 = sendMessage16(message,  HSPI_SS);

  #ifdef DEBGUG_SPI_RESULT
    Serial.print(result1, HEX);
    Serial.print(",\t\t");
    Serial.print( result1>>12, BIN);
    Serial.print(",\t");
    Serial.print( ((result1 & 0xF00)>>8), BIN);
    Serial.print(",\t");
    Serial.print( ((result1 & 0xF0)>>4), BIN);
    Serial.print(",\t");
    Serial.print( (result1 & 0xF), BIN);
    Serial.print(",\t\t");
  #endif
  
  resp = check_response(result1, init_pos);
  // Serial.print(">");
  // Serial.print(resp);
  // Serial.println("<");

  delay(100);
}

uint16_t check_response(uint16_t result1, uint16_t init_pos){
  uint16_t resultado;
  PAR = __builtin_popcount(result1);
  if (!(PAR % 2)){ // if equals zero, short version
    EF = (result1 & ErrorFlag_b) >> 1;
    if (!EF){
      AH = (result1 & AlarmHigh_b) >> 14;
      if(AH){
        Serial.print("Error Alarm High");
        Serial.print("\t");
      }
       AL = (result1 & AlarmLow_b) >> 15;
      if(AL){
        Serial.print("Error Alarm Low");
        Serial.print("\t");
      }
       #ifdef DEBGUG_ALARM
        Serial.print("\t");
        Serial.print(EF);
        Serial.print("<,\tH");
        Serial.print(AH);
        Serial.print(",\tL");
        Serial.print(AL);
        Serial.print(",\t");
      #endif
    
      resultado = (result1 & DataMask)>>2;

      if ((resultado == 4095) || (resultado==0)){
        if ((resultado == 4095) && (resultado_prev == 4095)){
          error = 1;
        }else if ((resultado == 0) && (resultado_prev == 0)){
          error = 1;
        }
      }

      if (error){
          Serial.println("Value Error");
          send_Reset(HSPI_SS);
      }else{
        resultado_prev = resultado;
        #ifdef DEBGUG_SPI
          Serial.print( resultado); 
          Serial.print(",\t");
          Serial.print( resultado - init_pos); 
          Serial.print(",\t");
        #endif
        Serial.print("Curr_val: ");
        Serial.print((float)(resultado)/11.38f);
        Serial.print("\t Offset_val: ");
        Serial.println((float)(resultado-init_pos)/11.38f);

        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      } 
    }else{
      Serial.println("Error Flag");
      send_Reset(HSPI_SS);
    }
  }else{  
    Serial.println("Error Parity");
    send_Reset(HSPI_SS);
  }
  return (float)(resultado)/11.38f;
}

void send_Reset(uint8_t SSpin){
  uint16_t mensa = 0x33A5;
  mensa |= __builtin_parity(mensa); //even
  digitalWrite(SSpin, LOW);
  result1 = SPI.transfer16(mensa);
  digitalWrite(SSpin, HIGH);
  Serial.println("<MASTER> reset"); 
  delay(10);
  error=0;
}

float sendMessage16(uint16_t mess, uint8_t SSpin){
  uint16_t result;
  mess |= __builtin_parity(mess); //even
  digitalWrite(SSpin, LOW);
  result = SPI.transfer16(mess);
  digitalWrite(SSpin, HIGH);
  #ifdef DEBGUG_SPI
    Serial.print(mess, HEX);
    Serial.print("\t");
  #endif
  return result;
}