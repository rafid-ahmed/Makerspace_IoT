# Makerspace Laser Cutter Fire Detection System

## Abstract
The purpose of our project is to enhance existing laser cutters in the [Makerspace](https://www.maker-space.de/Makerspace). The main issue we identified via expert interviews was that the laser cutter could be set on fire if wrong material is being cutten. Even a small fire could result into damaging the laser cutter. We developed a lightweight solution using ir sensors to identify fire, alarm the lasercutter user and stop the lasercutter as soon as possible to avoid damages.

## Functional description of Lasercutter.

The laser cutter we are equipping is the [VLS4.60](https://www.ulsinc.com/products/platforms/vls4-60) that can be seen in the picture below:
![Laser cutter](https://gitlab.com/tum-iot-lab/makerspace-lasercutter/raw/master/doc/sysfile00014378O8711_lo.jpg "Laser cutter")
The laser cutter is being used to cut or create drawings on [many surfaces](https://www.ulsinc.com/explore): 
![Laser cutter surfaces](https://gitlab.com/tum-iot-lab/makerspace-lasercutter/raw/master/doc/Screen%20Shot%202019-01-31%20at%2015.21.11.png "Laser cutter surfaces")

While offering this lasercutter, makerspace faces the following issues:
* Cutted material burns: Users sometimes use materials that are not suitable for this kind of laser or do not respect the cutting specification for the material that they are using. Makerspace does not actively track what materials people use and how they use it. A fire in the lasercutter could damage: 
    * The laser lens: 
    
      <img src="https://gitlab.com/tum-iot-lab/makerspace-lasercutter/raw/master/doc/sysfile00025137O5752_lo.jpg" alt="drawing" width="800"/>

    * The honey comb shaped ground 
      
      <img src="https://gitlab.com/tum-iot-lab/makerspace-lasercutter/raw/master/doc/WhatsApp%20Image%202019-01-28%20at%2014.05.23(1).jpeg" alt="drawing" width="800"/>

* The laser cutter does not offer any power consumption statistics

## Functional description of the developed solution.
-- Abstract for our solution --
Because of this, we have to build a system that can reliably identify fire from the very first moment it is triggered.


### Architecture (Noa)

-- Describe the architecture on a component diagram -- 

### Component analyzis
-- This chapter presents bla bla bla --

#### Sensors (Sergiu)
-- Present all the sensors we have with pictures --

##### Final sensor choice (Sergiu)
- show image with the final sensors and maybe explain why we have chosen this ones -

#### Board (Rafid)
-- Describe the board and which sensors we use, and how the things are wireded together --

#### Dasboard (Yamini)
-- Explain which technologies we have chosen and why we have chosen this technologies? Show the dashboard and explain each graph --

#### Makerspace MQTT (Rishab)

-- Explain how the Makerspace MQTT works, what data it gets and how they can use it --


Structure chart of the developed solution with the discussion about the decisions taken in the scope of the development.

## Flowchart of the developed System (Me & Rafid)
-- Flowchart of the developed System -- Explain how the system is working is working sensor captures data than that than that, than we look at treshold, if ir under 100 send fire event, data sent to mqtt bla bla --

## Setup (Sergiu, Noah, Rafid)
-- close-up foto with the sensor, explain what each pin does, and how to connect them --

## Conclusions (Rishab)

## Future Work (Rishab & Yamini)
