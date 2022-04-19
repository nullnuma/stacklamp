# StackLamp Prj

|Parts|Note|
|:--|:--|
|SW1|ESP8266 Enable|
|SW2|ESP8266 BootSelect or Wi-Fi Setting Btn|

## Wi-Fi 設定
起動直後3秒以内(赤点滅中)にSW2を押下することでWi-Fi設定APが起動する．  
SSIDが"StackLamp\[MACアドレス\]"，Passwordが"STACKLAMP!!"で接続可能．  
192.168.4.1にアクセスし設定後，電源を入れ直すことで適用される．

## 通常使用

LEDが赤点滅はWi-Fi設定モード待機中，青回転で接続中である．  
接続後は消灯．

LEDの表示はGETリクエストで状態の取得設定が可能．  

|URL|Info|
|:--|:--|
|/tower_info|現在の状態を取得|
|/tower_height|高さを取得|
|/tower|LEDの状態を設定可能|

### /towerについて

|layer|下からの高さ(n=0,...)|
|:--|:--|
|mode|表示の種類を設定(表示の種類参照)|
|hz|表示に伴う速度の設定|
|color|表示色の設定|(RRGGBB=000000-FFFFFF)||


以下は最下層を赤色で点滅する  

`/tower?layer=0&mode=2&color=ff0000&hz=1`

### 表示の種類
|No|Info|
|:--|:--|
|0|非表示|
|1|点灯|
|2|点滅|
|3|回転|
|4|虹色|
|5|虹色回転|
