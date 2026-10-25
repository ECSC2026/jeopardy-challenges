// strings.mc — String methods and character processing.

var text = "Hello, World";

println("Original:   " + text);
println("Length:     " + text.length());
println("Upper:      " + text.toUpper());
println("Lower:      " + text.toLower());
println("Substring:  " + text.substring(0, 5));
println("Find World: " + text.find("World"));
println("Char at 7:  " + text[7]);

// Count vowels by walking the characters.
function countVowels(s) {
    var vowels = "aeiouAEIOU";
    var count = 0;
    for (var i = 0; i < s.length(); i += 1) {
        if (vowels.find("" + s[i]) != null) {
            count += 1;
        }
    }
    return count;
}
println("Vowels:     " + countVowels(text));

// Reverse a string via its character array.
function reverse(s) {
    var chars = s.toCharArray();
    var result = "";
    for (var i = chars.size() - 1; i >= 0; i -= 1) {
        result += "" + chars[i];
    }
    return result;
}
println("Reversed:   " + reverse("VulnVM"));

// Parse numbers out of strings.
println("\"42\" + 8 = " + ("42".toNumber() + 8));
println("\"3.5\" as float = " + "3.5".toFloat());

// Build a simple CSV row.
var fields = ["name", "score", "rank"];
var row = "";
for (var i = 0; i < fields.size(); i += 1) {
    if (i > 0) { row += ","; }
    row += fields[i];
}
println("CSV row:    " + row);
