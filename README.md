# coffee-timer
Basic timer for tracking when to get coffee

# Compilation

```gcc -O2 -Wall -Wextra -o coffee-timer coffee_timer.c```

# Usage

## Get coffee every 25 minutes
```./coffee-timer 25m```

## Only run it once
```./coffee-timer --once 25m```

## Run in a loop with custom message

```./coffee-timer -m "Coffee time!" 45m```
