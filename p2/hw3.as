      lw    0 1  n
      lw    0 2  r
      lw    0 6  Caddr  // load combination fn address
      jalr  6 7         // call function
      halt
func  beq   0 2  base   // if r == 0
      beq   1 2  base   // if n == r
      lw    0 6  Neg1
      add   1 6  1      // n--
      lw    0 6  One
      sw    5 7  Stack  // save return address on stack
      add   5 6  5      // stack_ptr++
      <insert code here>
      lw    0 6  Caddr
      jalr  _____       // comb(n-1,r)
      add   0 3  4      // r4 = comb(n-1,r)
      lw    0 6  Neg1
      <insert code here>
      add   2 6  2      // r--
      lw    0 6  One
      <insert code here>
      lw    0 6  Caddr
      jalr  _____       // comb(n-1,r-1)
      lw    0 6  Neg1
      <insert code here>
      add   4 3  ___   // comb(n-1,r) + comb(n-1,r-1)
      lw    0 6  Neg1
      <insert code here>
      jalr  ____        // return 
base  lw    0 3  One    // return value of 1
      jalr  7 6         // return (base case)
Caddr .fill 5 
n     .fill 6
r     .fill 3
One   .fill 1
Neg1  .fill -1
