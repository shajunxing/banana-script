function outer()
    print(a, b, c)
end
function doit(func)
    func()
end
local a = 1
doit(function()
    local b = 2
    doit(function()
        local c = 3
        doit(function()
            print(a, b, c)
        end)
    end)
end)
doit(function()
    local b = 2
    doit(function()
        local c = 3
        doit(outer)
    end)
end)
doit(function()
    local b = 2
    doit(function()
        local c = 3
        local inner = outer
        doit(inner)
    end)
end)
