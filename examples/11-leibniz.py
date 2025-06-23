import time
start_time = time.clock()
loop = 1
while True:
    rounds = 1000000
    x = 1
    pi = 1.0
    i = 2
    while i < rounds + 2:
        x = -x
        pi = pi + (x / (2 * i - 1))
        i = i + 1
    pi = pi * 4
    print((time.clock() - start_time) / loop, "secs, pi is", pi)
    loop = loop + 1