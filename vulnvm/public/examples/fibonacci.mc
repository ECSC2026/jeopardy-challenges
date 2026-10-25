// fibonacci.mc — recursion, iteration, and memoization with a Dictionary.

// Classic recursive definition.
function fib(n) {
    if (n < 2) { return n; }
    return fib(n - 1) + fib(n - 2);
}

// Iterative version.
function fibIterative(n) {
    var a = 0;
    var b = 1;
    for (var i = 0; i < n; i += 1) {
        var next = a + b;
        a = b;
        b = next;
    }
    return a;
}

// Memoized version, sharing a cache dictionary.
var cache = {};
function fibMemo(n) {
    if (n < 2) { return n; }
    if (cache.hasKey(n)) { return cache.get(n); }
    var result = fibMemo(n - 1) + fibMemo(n - 2);
    cache.put(n, result);
    return result;
}

println("Recursive fib(10)  = " + fib(10));
println("Iterative fib(20)  = " + fibIterative(20));
println("Memoized  fib(30)  = " + fibMemo(30));

// Build the first 12 Fibonacci numbers into an array.
var seq = [];
for (var i = 0; i < 12; i += 1) {
    seq.add(fibIterative(i));
}
println("First 12: " + seq);
