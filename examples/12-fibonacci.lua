function createFib()
    return function(n, g)
        if (n <= 2) then
            return 1
        end
        return g(n - 1, g) + g(n - 2, g)
    end
end
local fib = createFib()
print(fib(36, fib))
