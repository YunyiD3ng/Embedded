/*——————————————————————————————————————————————————————————————————————————
    Mode Set for the gyro:
    FS = 245 dps.
    Typical raw data specification: 8.75 mdps/digit.
    ODR: 800Hz. Cut-off: 30Hz.
  =======================================================================  
    To meet the grading criteria:
    1.20% - Ability to successfully and continuously measure gyro values 
    from the angular velocity sensor.
      We use SPI to read the raw data from the gyro by using "read_data()"
      function. And we set a flag and time ticker to sample data for every
      0.5s.
    2.30% - Ability to convert measured data to forward movement velocity.
      In our function "move_v()", we We processed the raw data from the gyro 
      according to the datasheet and initially obtained the angular velocity.
      Based on human motion patterns, the opening and closing of the feet 
      during walking range from 0 to 5 degrees, which can be approximated as
      linear motion. Therefore, we adjusted parameters to obtain the linear 
      motion speed. Additionally, we created a circular buffer to store 
      velocity data for every 20 seconds.
    3.20% - Ability to calculate distance traveled.
      Based on the measured instantaneous speed, we perform calculations 
      similar to integration. In the function "abs_distance()", we calculate 
      the instantaneous movement distance based on the instantaneous speed, 
      and then sum up each calculated distance. This operation displays the 
      total movement distance in real time.
    4.15% - Creativity.
      In this part, we add more functions based on this project. First, we
      detect the user's walking steps by recognizing patterns in their 
      movement speed in function "measure_steps()". And the second part, we
      set a ticker to count the time spent when user start to use this project.
      The third part is to calculate the calories consumption. In this part,
      we firstly calculate the average speed of the user and auto choose its
      MET, which is a factor shows the exercise Intensity, and than calculate
      the calories consumption in the function "cal_consumption()". The final
      part is to display all of those useful infomation on the LCD screen in 
      real time, which contain: time, distance, total steps, and calories.
    5.15% - Well written and organized code.
————————————————————————————————————————————————————————————————————————————*/
#include <mbed.h>
#include <math.h>
#include "drivers/LCD_DISCO_F429ZI.h"
//Constants for Calculation
#define PI 3.1415926536
#define LEG_LENGTH 1      //unit meter
#define WEIGHT 70         //unit kilogram
#define BUFFER_SIZE 40    //Buffer size for velocity value
//Circular Buffer for velocity value in every 20 sec
double velocity_buffer[BUFFER_SIZE];
int buffer_index = 0;

volatile int flag     = 0;
volatile int steps    = 0;
volatile int cal_cs   = 0;
volatile int seconds  = 0;

int i = 0;

volatile double abs_dis  = 0;
volatile double move_velocity = 0;
//Set LCD
LCD_DISCO_F429ZI lcd;
//Set Pins for MOSI, MISO, SCLK of SPI
SPI spi(PF_9, PF_8, PF_7);
//Set Pin for CS
DigitalOut cs(PC_1);
//Set Flag for timer
void set_flag() {          
  flag = 1;
}

//Set mode of GYRO
void set_mode() {          
  cs=0;
  //Set ODR
  spi.write(0x20);
  //Mode: 11001111 (Output data rate selection:800Hz. Cut-Off:30Hz.)
  spi.write(0xCF);
  cs=1;
}

//I.20%-Ability to successfully and continuously measure gyro values from the angular velocity sensor
int16_t read_data(int code){
  //Read raw data and save Lower 8 bits to R_L
  cs = 0;
  spi.write(code);
  uint8_t R_L =spi.write(0x00);
  cs = 1;
  //Read raw data and save Upper 8 bits to R_H.
  cs = 0;
  spi.write(code+1);
  int8_t R_H =spi.write(0x00);
  cs = 1;
  int16_t raw_data= (R_H << 8)|R_L;

  return raw_data;
}

//II.30%-Ability to convert measured data to forward movement velocity
double move_v(int16_t raw_data){
  double ang_velocity;
  ang_velocity = 0.001*8.75*raw_data;//Convert raw data to angular velocity
  if(fabs(ang_velocity) > 0.9){
    move_velocity = PI*ang_velocity*LEG_LENGTH/180;//Calculate the forward movement velocity
    //printf("The angular velocity: %d deg/s\n", (int)ang_velocity);
    printf("Your movement velocity: %d m/s\n ", (int)move_velocity);
    //Store the data in the buffer
    velocity_buffer[buffer_index] = move_velocity;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
  }

  return move_velocity;
}

//III.20%-Ability to calculate distance traveled
void abs_distance(double velocity_data){
  //Calculate the absolute distance of moving by using the movement velocity
  abs_dis =  abs_dis + (fabs(velocity_data)/10)*1.5;
  printf("Your exercise distance: %d meters\n",(int)abs_dis);
}

//IV.15%-Creativity part: Measure the total steps of user while walking or running
void measure_steps(double t_velocity){
  static double pre_v = 0; // Store the previous speed
  static bool walk = false; // Status of movement
  // The threshold of speed
  const double low_speed_threshold = 0.1;
  const double high_speed_threshold = 0.5;
  // To monitor the movement pattern
  if (!walk && t_velocity > high_speed_threshold && pre_v <= low_speed_threshold){
    walk = true;
  }
  if (walk && t_velocity <= low_speed_threshold && pre_v > low_speed_threshold){
    steps++;
    walk = false;
  }
  //Reset pre_v for the next step.
  pre_v = t_velocity;
  printf("Your total steps is %d\n",steps);
}

//IV.15%-Creativity part: Counting the time using for walking.
void counting_time() {
    seconds++;
}

//Print the total use of time
void print_time() {
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    printf("Total time spent %02d:%02d:%02d\n", hours, minutes, secs);
}

//IV.15%-Creativity part: Calculate the Calorie consumption.
void cal_consumption(){
  double speed =0;
  double MET = 0;
  speed = abs_dis/seconds;
  if(speed<0.9){
    MET = 2.5;//light walking
    cal_cs = MET*WEIGHT*(seconds/3600);
  }
  else if(speed>=0.9 && speed<1.5){
    MET = 3.5;//normal walking
    cal_cs = MET*WEIGHT*(seconds/3600);
  }
  else if(speed>=1.5 && speed<1.8){
    MET = 4;//light running
    cal_cs = MET*WEIGHT*(seconds/3600);
  }
  else if(speed>=1.8 && speed<2.0){
    MET = 4.5;//normal running
    cal_cs = MET*WEIGHT*(seconds/3600);
  }
  printf("Calories Consumption: %d\n", cal_cs);
}

//IV.15%-Creativity part: Display data on LCD
void update_lcd_display(double abs_dis, int seconds, int steps, int cal_cs) {
  char buffer[80];
  //Set LCD and color
  lcd.Clear(LCD_COLOR_BLACK);
  lcd.SetBackColor(LCD_COLOR_BLACK);
  lcd.SetTextColor(LCD_COLOR_WHITE);
  //Set time for display
  int hours = seconds / 3600;
  int minutes = (seconds % 3600) / 60;
  int secs = seconds % 60;
  snprintf(buffer, sizeof(buffer), "Time: %02d:%02d:%02d", hours, minutes, secs);
  lcd.DisplayStringAt(0, LINE(5), (uint8_t *)buffer, CENTER_MODE);
  //Set movement distance for display
  snprintf(buffer, sizeof(buffer), "Distance: %d meters", (int)abs_dis);
  lcd.DisplayStringAt(0, LINE(7), (uint8_t *)buffer, CENTER_MODE);
  //Set steps for display
  snprintf(buffer, sizeof(buffer), "Total Steps: %d", steps);
  lcd.DisplayStringAt(0, LINE(9), (uint8_t *)buffer, CENTER_MODE);
  //Set Calories for display
  snprintf(buffer, sizeof(buffer), "Calories: %d", cal_cs);
  lcd.DisplayStringAt(0, LINE(11), (uint8_t *)buffer, CENTER_MODE);
}

int main() {
  cs=1;
  set_mode();
  //set SPI mode
  spi.format(8,3);
  spi.frequency(100000);
  //set data receive frequency 20Hz
	Ticker flag_t;
  flag_t.attach(&set_flag,500ms);
  //Set time
  Ticker time_t;
  time_t.attach(&counting_time,1000ms);

  lcd.Clear(LCD_COLOR_BLACK);
  lcd.SetBackColor(LCD_COLOR_BLACK);
  lcd.SetTextColor(LCD_COLOR_WHITE);

  while(1) {
    if(flag){
      int16_t dataZ = read_data(0xAC);
        /*******************************
        [Test for raw data receive]
        int16_t dataX = read_data(0xAA);
        int16_t dataY = read_data(0xA8);
        printf("Raw DataZ: %d\n",dataZ);
        printf("Raw DataX: %d\n",dataX);
        printf("Raw DataY: %d\n",dataY);
        ********************************/

      //Function: calculate forward movement velocity
      double v = move_v(dataZ);
      //Function: calculate the movement distance
      abs_distance(v);
      //Function: calculate the walking steps
      measure_steps(v);
      //Function: calculate the calories consumption
      cal_consumption();
      //Print time in TERMINAL
      print_time();
      //Function: display real time data of distance, time, steps, and calories consumption on the LCD
      update_lcd_display(abs_dis, seconds, steps, cal_cs);

      flag = 0; 
    }
  }
}