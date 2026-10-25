// collections.mc — Arrays and Dictionaries and their methods.

// --- Arrays ---
var nums = [5, 3, 8, 1, 9, 2];
println("Array:     " + nums);
println("Size:      " + nums.size());
println("First:     " + nums[0]);
println("indexOf 8: " + nums.indexOf(8));

nums.add(7);
println("After add: " + nums);

var doubled = [];
for (var i = 0; i < nums.size(); i += 1) {
    doubled.add(nums[i] * 2);
}
println("Doubled:   " + doubled);

nums.sort(null);
println("Sorted:    " + nums);
println("Reversed:  " + nums.reverse());
println("Slice 1-4: " + nums.slice(1, 4));

// Sum via a helper.
function sum(arr) {
    var total = 0;
    for (var i = 0; i < arr.size(); i += 1) { total += arr[i]; }
    return total;
}
println("Sum:       " + sum(nums));

// --- Dictionaries ---
var ages = {};
ages.put("Alice", 30);
ages.put("Bob", 25);
ages.put("Carol", 35);

println("Bob's age: " + ages.get("Bob"));
println("Has Dave:  " + ages.hasKey("Dave"));
println("Count:     " + ages.size());
println("Keys:      " + ages.keys().size() + " people");

// Dictionary with symbol keys (a small record).
var point = {:x => 10, :y => 20, :label => "origin-ish"};
println("Point x=" + point[:x] + " y=" + point[:y] + " (" + point[:label] + ")");

// Word frequency count.
var words = ["the", "cat", "the", "dog", "the", "cat"];
var freq = {};
for (var i = 0; i < words.size(); i += 1) {
    var w = words[i];
    if (freq.hasKey(w)) { freq.put(w, freq.get(w) + 1); }
    else { freq.put(w, 1); }
}
println("'the' appears " + freq.get("the") + " times");
println("'cat' appears " + freq.get("cat") + " times");
