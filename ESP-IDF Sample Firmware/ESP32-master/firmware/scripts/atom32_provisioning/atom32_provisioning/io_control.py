
from gpiozero import LEDBoard, Button

class IoControl:
    __OK_LED_PIN = 13
    __ERROR_LED_PIN = 19
    __PROGRESS_LED_PIN = 26
    __BUTTON_PIN = 6
    
    def __init__(self):
        self.led = LEDBoard(
                ok = self.__OK_LED_PIN,
                error = self.__ERROR_LED_PIN,
                progress = self.__PROGRESS_LED_PIN)
        self.button = Button(self.__BUTTON_PIN, pull_up = True, hold_time = 0.1)
        self.led.on()
        
    def wait_for_press(self):
        self.button.wait_for_press()
        
    def setOk(self):
        self.led.off()
        self.led.ok.on()
        
    def setError(self):
        self.led.off()
        self.led.error.on()
        
    def setProgress(self):
        self.led.off()
        self.led.progress.on()
 