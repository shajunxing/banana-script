import time
def bench_1():
    start_time = time.clock()
    round = 1000000
    i = 1
    while True:
        arr = []
        for j in range(round):
            arr.append(j * j)
        print((time.clock() - start_time) / i / round)
        i += 1
def bench_2():
    start_time = time.clock()
    round = 1000000
    i = 1
    while True:
        obj = {"foo" : [ {}, [] ]}
        for j in range(round):
            obj["foo"][0]["bar"] = j
            obj["foo"][1].append("Hello," + "World!")
        print((time.clock() - start_time) / i / round)
        i += 1
bench_2()