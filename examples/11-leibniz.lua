require('os')
local start_time = os.clock()
local loop = 1
while true do
    local rounds = 1000000
    local x = 1
    local pi = 1.0
    local i = 2
    while i < rounds + 2 do
        x = -x
        pi = pi + (x / (2 * i - 1))
        i = i + 1
    end
    pi = pi * 4
    print((os.clock() - start_time) / loop, 'secs, pi is', pi)
    loop = loop + 1
end
