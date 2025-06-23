require('os')
function bench_1()
    local start_time = os.clock()
    local round = 1000000
    local i = 1
    while true do
        local arr = {}
        local j
        for j = 0, round do
            arr[j] = j * j
        end
        print((os.clock() - start_time) / i / round)
        i = i + 1
    end
end
function bench_2()
    local start_time = os.clock()
    local round = 1000000
    local i = 1
    while true do
        local arr = {}
        local obj = {['foo'] = {{}, {}}}
        for j = 0, round do
            obj['foo'][1]['bar'] = j
            obj['foo'][2][j] = 'Hello,' .. 'World!'
        end
        print((os.clock() - start_time) / i / round)
        i = i + 1
    end
end
bench_2()
