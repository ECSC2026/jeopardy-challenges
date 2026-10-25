## Solution

Classical AES/GCM nonce reuse allows directly for tag forgery. All details explained in https://frereit.de/aes_gcm/.

solve.sage: Simon's finest hand-written solution.

solve-glm.py: GLM5.3-flash - use this for automating testing, it can fetch from website etc..

## Fun facts
* This challenge can be completely solved by just XORing three values. Which exactly is left as an exercise to the reader. The solution implemented in this folder is more general, but overkill for this challenge.
* You can solve this challenge by just copy-pasting values in https://frereit.de/aes_gcm/ - but this gets a bit painful.
* However, https://frereit.de/aes_gcm/ is very nice for explaining the solution, and even can sanity-check your implementation by comparing individual values.
* Greatest roadblock probably: Converting hex to polys in sage. FWIW, https://sylvainpelissier.gitlab.io/resources/2023/insoteaser/gcm/gcm.sage has wrong byte ordering :-/
* Also, you can use https://github.com/jvdsn/crypto-attacks/blob/master/attacks/gcm/forbidden_attack.py or similar.