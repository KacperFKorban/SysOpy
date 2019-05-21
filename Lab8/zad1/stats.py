import matplotlib.pyplot as plt

filters = [11, 31, 65]
threads = [1, 2, 4, 8]
block = [[0.971316, 0.494944, 0.286126, 0.272213]
	,[7.715541, 3.874533, 2.288479, 2.275155]
	,[33.378509, 17.838053, 10.225728, 10.100055]
	]
interleaved = [[0.989945, 0.493961, 0.335952, 0.320359]
	,[7.691050, 3.825099, 2.193540, 2.214718]
	,[33.210457, 17.643394, 10.070506, 10.082019]
	]

plt.plot(threads, block[0], 'b')
plt.plot(threads, block[1], 'g')
plt.plot(threads, block[2], 'r')
plt.plot(threads, interleaved[0], 'c')
plt.plot(threads, interleaved[1], 'm')
plt.plot(threads, interleaved[2], 'y')
plt.gca().legend(('block 11', 'block 31', 'block 65', 'interleaved 11', 'interleaved 31', 'interleaved 65'))
plt.show()

