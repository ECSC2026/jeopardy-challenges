// hello.mc — the basics: variables, arithmetic, strings, and output.

var name = "VulnVM";
var year = 2026;

println("Hello from " + name + "!");
println("The year is " + year + ".");

// The numeric tower: Number, Long, Float, Double.
var n = 21;
var big = 10000000000l;      // Long
var pi = 3.14159;            // Float
var precise = 2.718281828d;  // Double

println("Number:  " + (n * 2));
println("Long:    " + (big + big));
println("Float:   " + (pi * 2));
println("Double:  " + precise);

// Booleans, comparisons, ternary.
var a = 7;
var b = 12;
println("max(" + a + ", " + b + ") = " + ((a > b) ? a : b));
println("7 is " + ((a % 2 == 0) ? "even" : "odd"));

// Characters and formatting.
println("Letter: " + 'A' + " has code " + 'A'.toNumber());
println("Hex of 255: " + (255).format("0x%02X"));

var longVal = 1337l;
println("Hex of longVal: " + longVal.format("0x%08X"));