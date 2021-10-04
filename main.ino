#include <IRremoteESP8266.h>//8266的IRremote库，如果是arduinoIDE可以在“管理库”里下载(IRremoteESP8266,不是IRremote哈)
#include <IRrecv.h>//接收库，IRremoteESP8266里的，不用下载
#include <IRutils.h>//这个很重要，一定要include一下(resultToTimingInfo就是这个库的)。也是IRremoteESP8266里的,不用下载
#include <IRsend.h>//发射库，还是IRremoteESP8266里的,不用下载

//下面是一些设置，大概是一些容错率之类的参数，直接复制即可，在示例->第三方库示例->IRremoteESP8266->IRrecvDumpV2可以看到官方详细的解释，我们直接复制即可
const uint16_t kCaptureBufferSize = 1024;
#if DECODE_AC
const uint8_t kTimeout = 50;
#else   // DECODE_AC
const uint8_t kTimeout = 15;
#endif  // DECODE_AC
const uint16_t kMinUnknownSize = 12;
const uint8_t kTolerancePercentage = kTolerance;
#define LEGACY_TIMING_INFO false


const uint16_t kRecvPin = 14;//接收管的引脚（这里是D5脚(就是GPIO14，可百度引脚定义，这里填GPIO号)）

IRrecv irrecv(kRecvPin, kCaptureBufferSize, kTimeout, true);//传进所有的参数，固定写上即可（没有kCaptureBufferSize, kTimeout可以不填，只填kRecvPin也ok）
decode_results results;//固定写上即可，红外接收数据的results变量

const uint16_t kIrLed = 4;//发射的引脚(这里是D2脚(就是GPIO4，最好用这个引脚接发射管))
IRsend irsend(kIrLed);//传进发射引脚参数


void setup(){
  Serial.begin(115200);//串口的波特率，不一样的话串口会乱码
  irsend.begin();//初始化irsend发射的这个库，必须要有
  irrecv.enableIRIn();     // 初始化irrecv接收的这个库，必须要有
  while (!Serial)          //等待初始化完成，防止莫名其妙的bug   
    delay(55);
}


uint16_t  rawData_Results[555]; //存放红外发射用的raw码数组，注意必须是uint16_t这个类型(这里研究了好久233)，然后长度设置长一点，300左右差不多，我这比较夸张

void loop() {
  if (irrecv.decode(&results)) {          //判断是否接收到了红外的信号
    /*这里说一下
     resultToSourceCode这个是IRutils里的，没有用到results的参数，results的参数实在没搞懂，突然发现了这个，研究了一下，他输出的内容大概是这样的（String类型的）
     #uint16_t rawData[67] = {8782, 4518,  544, 584,  540, 584,  538, 586,  562, 562,  566, 582,  540, 582,  542, 584,  538,...一堆数组（太长了不写了）}#
     我们用到的就是这个uint16_t数组的长度[int]:和后面的数组(注意中间有空格，需要先去空格)
     */
    
    String IRrec_value_raw = resultToSourceCode(&results);//把resultToSourceCode到String类型名字是IRrec_value_raw变量(变量名随意哈)
    
    //下面是截取[int]的int
    int IRrec_value_tap_start = IRrec_value_raw.indexOf("rawData[") + 8;//lastIndexOf从前往后寻找String中的对应字符位置，indexOf是String的，说白了就是寻找这些字符返回他们的位置，这里算好了位置了，可以自己Serial.print出resultToSourceCode研究研究哈
    int IRrec_value_tap_over = IRrec_value_raw.indexOf("]");//寻找String中"]"的位置
    int IRrec_value_tap = IRrec_value_raw.substring(IRrec_value_tap_start,IRrec_value_tap_over).toInt();//substring也是String的，说白了就是截取两个参数(字的位置)中间的字符到String里，因为正好是数字，所以直接.toInt了，这就是raw码数组的长度
    //下面是截取数组的字符串，不包含大括号
    int IRrec_value_raw_start = IRrec_value_raw.indexOf("{") +1 ;//寻找String中{的位置，加一正好是数字开始
    int IRrec_value_raw_over = IRrec_value_raw.indexOf("}") ;//寻找结尾的位置
    String IRrec_Raw_Value = IRrec_value_raw.substring(IRrec_value_raw_start,IRrec_value_raw_over);//拼合起来，这里还是有空格的哈
    irrecv.resume();     //接收下一个值，必须要加
    
//下面开始把String的数据转到uint16_t这个类型里，可能变量名比较长反而看不懂，可以自己替换一下之后再理解
    int rawList_start = 0;//记录之后for的时候每组数字的时候的开始位置的变量
    int rawList_over = -1;///记录之后for的时候每组数字的时候的结束位置的变量
    int rawList_result;//记录每组数据的变量
    int tap = IRrec_value_tap - 1;//for从0开始，所以-1
    
    IRrec_Raw_Value.replace(" ", "");//删掉那个寻找出来String类型的raw数组里面的那些空格，把" "替换成""
    
    for(int i=0;i<=tap; i++){
      rawList_start = rawList_over + 1;//每次加上次的1，现在变量里已经没有空格了，只隔一个逗号就是下一组数据的开始位置，初始是0
      rawList_over = IRrec_Raw_Value.indexOf(",",rawList_start);//寻找逗号位置，从第二个参数开始(上一次的开始位置开始)
      rawList_result = IRrec_Raw_Value.substring(rawList_start,rawList_over).toInt();//截取每组数据，int形式暂存里
      rawData_Results[i] = rawList_result;//添加到这个uint16_t数组里
   }
   //下面是测试部分，可以自行修改哈
   //这里说白了就是按完了就不停的发送学习的数据，如果你的电器有反应就学习成功了
   //看懂这些代码之后自己改吧，比如接入个blinker什么的，各位自行发挥哈哈哈
   while(1){
      irsend.sendRaw(rawData_Results,IRrec_value_tap,38);//这是发送Raw数组，第一个参数就是uint16_t类型的数组，第二个是数组长度，第三个是频率，红外一般就是38kHZ，直接填上38就好
      delay(500);//每次发送，delay个半秒
      }
}
}      
