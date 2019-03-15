import matplotlib.pyplot as plt

f = open("raw_data.txt", "r")

xs = []
sys = []
lib = []

for line in f:
    tokens = line.split(" ")
    x = int(tokens[1])
    if x not in xs:
        xs.append(x)
    if(tokens[0] == "sys"):
        sys.append(float(tokens[2]))
    else:
        lib.append(float(tokens[2]))

f.close();

plt.plot(xs, sys, 'r')
plt.plot(xs, lib, 'b')
plt.gca().legend(('sys', 'lib'))
plt.show()
