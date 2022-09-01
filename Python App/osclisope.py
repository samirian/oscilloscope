import matplotlib.pyplot as plt
import matplotlib.animation as animation
import time
import numpy as np
from scipy.interpolate import spline

from ulib import USB


SAMPLE_COUNT = 50
vid = "16c0"
pid = "04dc"
interface = "0"
usb = USB.USB(vid, pid, interface)
ECHO = 3
READ = 2
WRITE = 4
#control test

data = bytearray()
l = 0XFF
h = 0X60
a = l + h*0x100
prescaler = 1
data.append(prescaler)
data.append(l)
data.append(h)
time.sleep(1)

print(data)
print(usb.readControl(ECHO,0xC0,0,0, 3,data))
usb.writeControl(WRITE,0x40,0,0,3,data)
print(usb.readControl(ECHO,0xC0,0,0, 3,data))
time.sleep(1)

temp = bytearray()
for i in range(SAMPLE_COUNT + 1):
	temp.append(0)
n = 10

tempo = bytearray()

for i in range(SAMPLE_COUNT):
	tempo.append(255)
tempo[0] = 0

plt.ion()
fig = plt.figure()
plt.xlabel('ms')
plt.ylabel('v')
ax = fig.add_subplot(111)

def read():
	while True:
		try:
			temp_signal = usb.readControl(2,0xC0,0,0, SAMPLE_COUNT + 1,temp)
			break
		except:
			print("device busy")
			continue
	return  temp_signal[0], temp_signal[1:]


x_l = np.linspace(0, 50*2*(1+a)/(12000), SAMPLE_COUNT)
# x_smooth = np.linspace(0, 10*2*(1+a)/12000 ,50)
# sine_smooth = spline(x_l, tempo, x_smooth)
# line1, = ax.plot(x_smooth, sine_smooth, 'b-') # Returns a tuple of line objects, thus the comma

line1, = ax.plot(x_l, tempo, 'b-') # Returns a tuple of line objects, thus the comma

while True:
	signal = []
	x = []
	count = 0
	while count < SAMPLE_COUNT:
		time.sleep(0.5)
		print(count)
		count, samples = read()

	signal = samples[0:]
	# sine_smooth = spline(x_l, signal, x_smooth)
	# line1.set_ydata(sine_smooth)

	line1.set_ydata(signal)
	line1.set_xdata(x_l)
	fig.canvas.draw()