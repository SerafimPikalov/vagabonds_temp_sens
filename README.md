# vagabonds_temp_sens
Software for the ESP8266 microcontroller, designed to control a LED thermometer for the sauna at the Vagabonds theme camp during AfrikaBurn

# Setting up and configuration
  1. Attach any 5-meter LED strip from the box to the wooden thermometer. Double-sided duct tape is fine, just duct tape too. Just make it reversible.
  2. Connect to Wifi - Vagabonds_sauna_tempr (pass: warmitup). If webpage not opens automaticly go to http://8.8.8.8
  3. Count how many LEDs from the tail (where all wires are) to the place where the sign 0C is on the thermometer. Put this value in First LED num field on website
  4. Count how many LEDs to the end of the scale. Put this value in "Last LED num";
  5. Ask the sauna lead what the minimum temperature is considered good enough and put this in "Red zone from temp"
  6. Warm up the sauna to the minimum acceptable temperature by feeling. Go to this page and check the temperature sensor value. If it's less than 'Red zone from temp', put the delta between the numbers in "sensor correction"

  # FAQ
  Yes powersupply could make noises - it's fine, but always check power consumption.
  LED strips is interchangeable - any 5V will work.