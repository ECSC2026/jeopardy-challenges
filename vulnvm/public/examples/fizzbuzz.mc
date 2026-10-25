// fizzbuzz.mc — control flow: for loops, if/else chains, while, and switch.

function fizzbuzz(n) {
    if (n % 15 == 0) { return "FizzBuzz"; }
    if (n % 3 == 0)  { return "Fizz"; }
    if (n % 5 == 0)  { return "Buzz"; }
    return "" + n;
}

for (var i = 1; i <= 20; i += 1) {
    println(fizzbuzz(i));
}

println("---");

// A while loop counting down.
var countdown = 5;
while (countdown > 0) {
    print(countdown + " ");
    countdown -= 1;
}
println("liftoff!");

// A switch statement with fallthrough.
function describe(day) {
    switch (day) {
    case 0:
    case 6:
        return "weekend";
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        return "weekday";
    default:
        return "unknown";
    }
}
println("Day 0 is a " + describe(0));
println("Day 3 is a " + describe(3));
println("Day 9 is " + describe(9));

// break and continue.
var sumOfOdds = 0;
for (var k = 0; k < 100; k += 1) {
    if (k % 2 == 0) { continue; }
    if (k > 10) { break; }
    sumOfOdds += k;
}
println("Sum of odds 1..9 = " + sumOfOdds);
