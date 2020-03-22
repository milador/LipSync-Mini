# LipSync-Mini

Lipsync with updated electronics (Adafruit ItsyBitsy and RGB LED)

<p align="center">
<img align="center" src="https://raw.githubusercontent.com/makersmakingchange/blog/gh-pages/_resources/images/LipSync_Logo.jpg" width="50%" height="50%" alt="LipSync Logo"/>
</p>

The LipSync is an assistive technology device developed by <a href="https://www.makersmakingchange.com/" download target="_blank">MakersMakingChange</a> (An initiative by the <a href="https://www.neilsquire.ca/" download target="_blank">Neil Squire Society</a>) which allows quadriplegics and other people with limited hand use the ability to use touchscreen mobile devices by manipulation of a mouth-operated joystick with integrated sip and puff controls. We are releasing all of our work open-source, to make the Lipsync a solution that can be made at the community level for less than $300.

<p align="center">
<img align="center" src="https://raw.githubusercontent.com/milador/LipSync-Mini/master/Resource/LipSync-Mini.jpg" width="50%" height="50%" alt="LipSync Logo"/>
</p>

## Main LipSync github repository 

The LipSync Mini project is based on the <a href="https://github.com/makersmakingchange/LipSync" download target="_blank">LipSync</a>


## Changes Made

1. Dual color Led replaced by RGB addressable 
2. Arduino Micro replaced by Adafruit ItsyBitsy M0 (More memory and power)
3. Transistor and header pins for bluetooth module removed 
4. ItsyBitsy nRF52840 Express with bluetooth 5.0 is used for bluetooth version (Adafruit ItsyBitsy M0 replaced by ItsyBitsy nRF52840 Express)
5. Option to add ESP8266 Wifi Module to enable controlling IOT devices 
6. Option to add internal battery charger
7. Option to use internal input 3.5mm jack switch 
8. Option to replace sip and puff module with bite switch PCB
