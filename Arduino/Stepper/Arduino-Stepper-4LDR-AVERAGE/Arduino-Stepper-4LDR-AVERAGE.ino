// Projeto Girassol - servomotor
// circuito de leitura de voltagem: 5V - Resistência variável (Rx) - Resistência de referencia (R0 = 1000 Ohms) - GND

#include <Stepper.h>

// altere isso para ficar de acordo com a quantidade de passos por volta do seu motor de passo
const int stepsPerRevolution = 2048;

Stepper stepper_x(stepsPerRevolution, 4, 5, 6, 7);
Stepper stepper_y(stepsPerRevolution, 8, 9, 10, 11);

//posição inicial
int PX=90;
int PY=90;

/* dobro da leitura típica do fotorresistor à noite 
(para que o arduino nao precise fazer mais uma operação ao comparar a leitura média com a leitura noturna na função enoughLight() definida abaixo */
const long int doubleR_nite=600000; // em Ohms
bool niteMode=true;

// Valores de leitura dos resistores responsáveis pela movimentação no eixo X - de frente para o motor: R1 - Eixo X - R2
float read1,read2;
float R1,R2;
float R1R2,IXB_IXA;

// Valores de leitura dos resistores responsáveis pela movimentação no eixo X - de frente para o motor: R3 - Eixo Y - R4
float read3,read4;
float R3,R4;
float R3R4,IYB_IYA;

// resistência de referência (em Ohms)
float R0=1000;

// tolerância de verificação (um pode ser 30% maior que o outro: TV_Max=1.1 ou TV_Min=1/1.1)
float TV_Min=0.909091;
float TV_Max=1.1;

// tolerância de ajuste (movimentação) - (um pode ser 10% maior que outro: TA_Max=1.05 ou TA_Min=1/1.05)
float TA_Min=0.952381;
float TA_Max=1.05;

// log(R)= -a*log(I)+log(b) onde a é positivo - valor obtido experimentalmente - R=b.I^(-a)
float a=0.648;

// intervalo entre os "passos" do servo motor (milissegundos)
const int t=25;

// intervalo entre medições à noite e ao dia (minutos) e elemento de iteração i
const int niteInterval=30;
const int dayInterval=1;
int i;

void setup() {

  // definir velocidade em RPM
  stepper_x.setSpeed(10);
  stepper_y.setSpeed(10);
  
  pinMode(1,INPUT);
  pinMode(2,INPUT);

  pinMode(3,INPUT);
  pinMode(4,INPUT);

  // descomentar linha(s) abaixo para observar os valores das resistências no Serial Monitor
  //Serial.begin(9600);
}

// curvas de calibração devem ser obtidas experimentalmente
void calibrate(int i){
  switch (i) {
    case 2:
      R2 = 1.0*R2+0.0;
      break;
    case 3:
      R3 = 1.0*R3+0.0;
      break;
    case 4:
      R4 = 1.0*R4+0.0;
      break;
  }
}

void readResistors() {
  read1=analogRead(1);
  read2=analogRead(2);
  read3=analogRead(3);
  read4=analogRead(4);
  R1=R0*(1023.01-read1)/(read1+0.01);
  R2=R0*(1023.01-read2)/(read2+0.01);
  R3=R0*(1023.01-read3)/(read3+0.01);
  R4=R0*(1023.01-read4)/(read4+0.01);
}

void evaluate_x() {
  // descomentar linha(s) abaixo para observar os valores das resistências no Serial Monitor
  // Serial.print("R1 = ");Serial.println(R1);
  // Serial.print("R2 = ");Serial.println(R2);
  calibrate(2);
  // descomentar linha(s) abaixo para observar os valores das resistências no Serial Monitor
  // Serial.print("R2_cal = ");Serial.println(R2);
  R1R2=(R1+R4)/(R2+R3);
  IXB_IXA=pow(R1R2,(1/a));
}

void evaluate_y() {
  // descomentar linha(s) abaixo para observar os valores das resistências no Serial Monitor
  // Serial.print("R3 = ");Serial.println(R3);
  // Serial.print("R4 = ");Serial.println(R4);
  calibrate(3);
  calibrate(4);
  // Serial.print("R3_cal = ");Serial.println(R3);
  // Serial.print("R4_cal = ");Serial.println(R4);
  // Serial.println();
  R3R4=(R3+R4)/(R1+R2);
  IYB_IYA=pow(R3R4,(1/a));
}

void step_x() {
  if (IXB_IXA<TA_Min && PX<180){
    PX=PX+1;
    stepper_x.step(-1);
  }
  else if (IXB_IXA>TA_Max && PX>0){
    PX=PX-1;
    stepper_x.step(1);
    }
}

void step_y() {
  if (IYB_IYA<TA_Min && PY<180){
    PY=PY+1;
    stepper_y.step(-1);
  }
  else if (IYB_IYA>TA_Max && PY>0){
    PY=PY-1;
    stepper_y.step(1);
  }
}

void sleep_x() {
  stepper_x.step(PX-90);
  PX=90;
}

void sleep_y() {
  stepper_y.step(PY-90);
  PY=90;
}

bool enoughLight(){
  return ((R1+R2+R3+R4)<(2*doubleR_nite));
}

void wait(int minutos){
  for (i=0; i<minutos; i++){
    delay(60000);
  }
}

void loop() {
  
  while(niteMode){
    readResistors();
    evaluate_x();
    evaluate_y();
    if (enoughLight()){
      niteMode=false;
    }
    else {
      wait(niteInterval);
    }
  }

  while (IXB_IXA<TV_Max && IXB_IXA>TV_Min && IYB_IYA<TV_Max && IYB_IYA>TV_Min){
    wait(dayInterval);
    readResistors();
    evaluate_x();
    evaluate_y();
  }

  while((IXB_IXA>TA_Max || IXB_IXA<TA_Min || IYB_IYA>TA_Max || IYB_IYA<TA_Min) && enoughLight()){
    step_x();
    step_y();
    delay(t);
    readResistors();
    evaluate_x();
    evaluate_y();
  }
  
  if (!enoughLight()){
    sleep_x();
    sleep_y();
    niteMode=true;
  }

}
