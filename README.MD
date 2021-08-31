# ESP8266 低成本WiFiDuck，基于Spacehuhn WiFiDuck项目，使用CH9328作为USB HID设备  
原项目链接：https://github.com/SpacehuhnTech/WiFiDuck  
PCB开源地址：https://oshwhub.com/lxu0423/ch9328-wifi  

# 重要说明（对于第一版PCB，如果以后使用了软件串口再改）

## 接好滤波电容，接好滤波电容，接好滤波电容！！！大于100uf，不然很可能不稳定，万一复位键盘就会输入奇怪的东西  

## 使用配置工具配置CH9328的波特率为38400  
工具在立创开源广场的附件，也可以在[这个repo](https://github.com/Edragon/WCH-CH9328)里面的`CH9328EVT/EVT/TOOLS/CH9328CFG/`目录中找到
