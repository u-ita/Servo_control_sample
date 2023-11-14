/************************************************************/
// Arduino用S.BUS通信プログラム 竹中版
// 
// トリム機能
// 断線検出
// 速度制限
/************************************************************/
#define SBUS_SPEED    100000     //SBUS通信速度
#define SPEED_LIMIT   3.0       //サーボの最大速度制限

char sbus_data[25] = {
  0x0f, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00
};

double trim_x;
double trim_y;
double last_angle_x;  //速度制限用、前回の角度を保存

void setup() {
  Serial.begin(SBUS_SPEED, SERIAL_8E2); // 100kbps 1データ8bit　パリティEven StopBit:2bit
  /* スイッチ読み取りポートをプルアップ=5Vに引き上げ */
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  /* トリムの初期化 */
  trim_x = 0.0;
  trim_y = 0.0;
  /* 前回角度を初期化 */
  last_angle_x = 0.0;
}

void loop() {
  double  target_angle[16];                             //指令角度格納配列 unit[deg.]
  short  sbus_servo_id[16];                            //送信用指令角度格納配列（SBUS用変換後）
  int i = 0;

  /* トリム処理 */
  if(digitalRead(2) == 0 && trim_x <= 30.0 ){
    trim_x = trim_x + 0.25;
  }else if(digitalRead(3) == 0 && trim_x >= -30.0){
    trim_x = trim_x - 0.25;
  }
  if(digitalRead(4) == 0 && trim_y <= 30.0){
    trim_y = trim_y + 0.25;
  }else if(digitalRead(5) == 0 && trim_y >= -30.0){
    trim_y = trim_y - 0.25;
  }

  /* ジョイスティックの電圧読み取り */
  double temp = analogRead(A0);
  double temp2 = analogRead(A1);
  
  /* 断線検出（断線＝以上電圧を検出したら中立に固定） */
  if(temp2 < 40.0 || 950.0 < temp2){
    temp2 = 474.0;
  }
  
  /* IDごとの角度入力、電圧読み取り値(0~1023)を不感帯を切り取って-90~+90に変換 */
  target_angle[0] = (temp2 - 83.0 ) / 845.0 * 180.0 - 90.0 + trim_x;
  target_angle[1] = 0.0;
  target_angle[2] = 0.0;
  target_angle[3] = 0.0;
  target_angle[4] = 0.0;
  target_angle[5] = 0.0;

  /* 角度範囲を-90～+90に限定（はみ出したら範囲内に戻す） */
  for (i = 0; i <= 5; i++) {
    if(target_angle[i] > 90.0){
      target_angle[i] = 90.0;           // 90以上なら90で上書き
    }else if(target_angle[i] < -90.0){
      target_angle[i] = -90.0;          // -90以下なら-90で上書き
    }
  }

  /* 速度制限処理 */
  double difference_x = target_angle[0] - last_angle_x;
  if(difference_x > SPEED_LIMIT){
    difference_x = SPEED_LIMIT;
  }else if(difference_x < (-1.0 * SPEED_LIMIT)){
    difference_x = (-1.0 * SPEED_LIMIT);
  }
  target_angle[0] = last_angle_x + difference_x;
  last_angle_x = target_angle[0];
  
  /*------   目標角度[deg.]をS.BUS用に変換      -----*/
  for (i = 0; i <= 15; i++) {
    // -90~+90deg を 0~2047 に変換
    sbus_servo_id[i] = (int)(  (double)(target_angle[i] + 90.0) / 180.0 * 2047.0 );
  }
  
  /* sbus送信用に変換した目標角度をS.BUS送信用バッファに詰め込む  */
  sbus_data[0] = 0x0f;
  sbus_data[1] = ( sbus_servo_id[0] & 0xff);
  sbus_data[2] = ((sbus_servo_id[0] >> 8) & 0x07 ) | ((sbus_servo_id[1]  << 3 ) );
  sbus_data[3] = ((sbus_servo_id[1] >> 5) & 0x3f ) | (sbus_servo_id[2]  << 6);
  sbus_data[4] = ((sbus_servo_id[2] >> 2) & 0xff ) ;
  sbus_data[5] = ((sbus_servo_id[2] >> 10) & 0x01 ) | (sbus_servo_id[3]  << 1 )   ;
  sbus_data[6] = ((sbus_servo_id[3] >> 7) & 0x0f ) | (sbus_servo_id[4]  << 4 )   ;
  sbus_data[7] = ((sbus_servo_id[4] >> 4) & 0x7f ) | (sbus_servo_id[5]  << 7 )   ;
  sbus_data[8] = ((sbus_servo_id[5] >> 1) & 0xff )   ;
  sbus_data[9] = ((sbus_servo_id[5] >> 9) & 0x03 ) ;

  /* sbus_dataを25bit分送信 */  
  Serial.write(sbus_data, 25);
  
  /* 制御周期コントロール 10msec待機＝約100Hz弱 */
  delay(10);
}
