# coffee-timer
Basic timer for tracking when to get coffee

# Compilation

```gcc -O2 -Wall -Wextra -o coffee-timer coffee.c```

# Usage

## Get coffee every X minutes
```./coffee-timer Xm```

## Only run it once
```./coffee-timer --once 25m```

## Run in a loop with custom message

```./coffee-timer -m "Coffee time!" 45m```
