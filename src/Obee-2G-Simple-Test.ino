
#include "Adafruit_DHT.h"
#include "ArduinoJson.h"
//#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"

#define DHTPIN D2     // what pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11		// DHT 11

/**
 * Simple class to monitor for has power (USB or VIN), has a battery, and is charging
 *
 * Just instantiate one of these as a global variable and call setup() out of setup()
 * to initialize it. Then use the getHasPower(), getHasBattery() and getIsCharging()
 * methods as desired.
 */
class PowerCheck
{
public:

	PowerCheck();
	virtual ~PowerCheck();

	/**
	 * You must call this out of setup() to initialize the interrupt handler!
	 */
	void setup();

	/**
	 * Returns true if the Electron has power, either a USB host (computer), USB charger, or VIN power.
	 *
	 * Not interrupt or timer safe; call only from the main loop as it uses I2C to query the PMIC.
	 */
	bool getHasPower();

	/**
	 * Returns true if the Electron has a battery.
	 */
	bool getHasBattery();

	/**
	 * Returns true if the Electron is currently charging (red light on)
	 *
	 * Not interrupt or timer safe; call only from the main loop as it uses I2C to query the PMIC.
	 */
	bool getIsCharging();

private:
	void interruptHandler();

	PMIC pmic;
	volatile bool hasBattery = true;
	volatile unsigned long lastChange = 0;
};

PowerCheck::PowerCheck() {
}

PowerCheck::~PowerCheck() {
}

void PowerCheck::setup() {
	// This can't be part of the constructor because it's initialized too early.
	// Call this from setup() instead.

	// BATT_INT_PC13
	attachInterrupt(LOW_BAT_UC, &PowerCheck::interruptHandler, this, FALLING);
}

bool PowerCheck::getHasPower() {
	// Bit 2 (mask 0x4) == PG_STAT. If non-zero, power is good
	// This means we're powered off USB or VIN, so we don't know for sure if there's a battery
	byte systemStatus = pmic.getSystemStatus();
	return ((systemStatus & 0x04) != 0);
}

/**
 * Returns true if the Electron has a battery.
 */
bool PowerCheck::getHasBattery() {
	if (millis() - lastChange < 100) {
		// When there is no battery, the charge status goes rapidly between fast charge and
		// charge done, about 30 times per second.

		// Normally this case means we have no battery, but return hasBattery instead to take
		// care of the case that the state changed because the battery just became charged
		// or the charger was plugged in or unplugged, etc.
		return hasBattery;
	}
	else {
		// It's been more than a 100 ms. since the charge status changed, assume that there is
		// a battery
		return true;
	}
}


/**
 * Returns true if the Electron is currently charging (red light on)
 */
bool PowerCheck::getIsCharging() {
	if (getHasBattery()) {
		byte systemStatus = pmic.getSystemStatus();

		// Bit 5 CHRG_STAT[1] R
		// Bit 4 CHRG_STAT[0] R
		// 00 – Not Charging, 01 – Pre-charge (<VBATLOWV), 10 – Fast Charging, 11 – Charge Termination Done
		byte chrgStat = (systemStatus >> 4) & 0x3;

		// Return true if battery is charging if in pre-charge or fast charge mode
		return (chrgStat == 1 || chrgStat == 2);
	}
	else {
		// Does not have a battery, can't be charging.
		// Don't just return the charge status because it's rapidly switching
		// between charging and done when there is no battery.
		return false;
	}
}

void PowerCheck::interruptHandler() {
	if (millis() - lastChange < 100) {
		// We very recently had a change; assume there is no battey and we're rapidly switching
		// between fast charge and charge done
		hasBattery = false;
	}
	else {
		// Note: It's quite possible that hasBattery will be false when there is a battery; the logic
		// in getHasBattery() takes this into account by checking lastChange as well.
		hasBattery = true;
	}
	lastChange = millis();
}



// Global variables
PowerCheck powerCheck;

DHT dht(DHTPIN, DHTTYPE);
FuelGauge fuel;

double publishTime = 900000; //15 min
//Usado para los sensores de temperatura. Debe ser menor o igual al PublishTime.
//Se utiliza si el tiempo de Publish es muy grande, para detectar "EVENTOS"


unsigned long ms;
unsigned long msLast;

SerialLogHandler logHandler;

#define OLED_RESET D7
Adafruit_SSD1306 display(OLED_RESET);


#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup() {

  Serial.begin(9600);
  dht.begin();
  powerCheck.setup();

	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

	display.display(); // show splashscreen
  delay(2000);
  display.clearDisplay();   // clears the screen and buffer

	// text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Hello, Obee!");
  display.setTextColor(WHITE);
	display.println("Conectando...");
  display.display();
  delay(2000);

}

int varDisplay = 0;

void loop() {

  delay(2000);

  ms = millis();

  // very slow sensor)
	float h = dht.getHumidity();
// Read temperature as Celsius
	float t = dht.getTempCelcius();
// Read temperature as Farenheit

  if (isnan(h) || isnan(t)) {
    Log.error("Failed to read from DHT sensor!");
    return;
  }

  float b = fuel.getSoC(); //0 to 100

  bool s = powerCheck.getHasPower();

  Log.info("Humid: " + String(h));
	Log.info("Temp: " + String(t));
	Log.info("Bat: " + String(b, 2));
  Log.info("Status: " + String(s));

	//varDisplay
	if(varDisplay == 0)
	{
		display.clearDisplay();   // clears the screen and buffer

		// text display tests
	  display.setTextSize(1);
	  display.setTextColor(WHITE);
	  display.setCursor(0,0);
	  display.println("Temp C - " + String(t,2));

		// text display tests
	  display.setTextSize(1);
	  display.setTextColor(WHITE);
	  display.println("Humedad % - " + String(h,2));

		display.display();

	  varDisplay = 1;
	}
	else if (varDisplay == 1)
	{
		display.clearDisplay();   // clears the screen and buffer

		// text display tests
	  display.setTextSize(1);
	  display.setTextColor(WHITE);
	  display.setCursor(0,0);
	  display.println("Bateria % - " + String(b,2));

		// text display tests
	  display.setTextSize(2);
	  display.setTextColor(WHITE);
		if(s)
			{
				display.println("CARGANDO...");
			}
			else
			{
				display.setTextColor(BLACK,WHITE);
				display.println("CORTE ENERGIA!");
			}

			display.display();

			varDisplay = 0;
	}


  if(ms - msLast > publishTime)
  {
    msLast = ms;

    StaticJsonBuffer<1024> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    root["f1"] = t;
    root["f2"] = h;
    root["b"] = round(b*10)/10;
    root["s"] = s;

    // Get JSON string.
    char buffer[1024];
    root.printTo(Serial);
    root.printTo(buffer, sizeof(buffer));

    //Serial.println("---------------");

    //Publicacion - Losant toma el evento del API de Particle
    Particle.publish("Publish", buffer , PRIVATE, NO_ACK);

  }



}
