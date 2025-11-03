#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>

#ifdef _WIN32
#include <windows.h>
#endif

static volatile sig_atomic_t g_running = 1;

static void handle_sigint(int signo)
{
    (void)signo;
    g_running = 0;
}

static void install_signal_handlers(void)
{
#ifndef _WIN32
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
#else
    signal(SIGINT, handle_sigint);
#endif
}

static void sleep_seconds(int secs)
{
#ifdef _WIN32
    Sleep(secs * 1000);
#else
    struct timespec ts;
    ts.tv_sec = secs;
    ts.tv_nsec = 0;
    while (nanosleep(&ts, &ts) == -1 && g_running)
    {
    }
#endif
}

static void sleep_milliseconds(int ms)
{
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && g_running)
    {
    }
#endif
}

static long long parse_hms_colon(const char *s, int *ok)
{
    int a = -1, b = -1, c = -1;
    char extra;
    *ok = 0;
    if (sscanf(s, "%d:%d:%d %c", &a, &b, &c, &extra) == 3)
    {
        if (a < 0 || b < 0 || b > 59 || c < 0 || c > 59)
            return -1;
        *ok = 1;
        return (long long)a * 3600LL + (long long)b * 60LL + (long long)c;
    }
    if (sscanf(s, "%d:%d %c", &a, &b, &extra) == 2)
    {
        if (a < 0 || b < 0 || b > 59)
            return -1;
        *ok = 1;
        return (long long)a * 60LL + (long long)b;
    }
    return -1;
}

static long long parse_duration(const char *s)
{
    while (isspace((unsigned char)*s))
        s++;
    if (*s == '\0')
        return -1;

    int ok = 0;
    long long colon = parse_hms_colon(s, &ok);
    if (ok)
        return colon;

    long long total = 0;
    const char *p = s;
    while (*p)
    {
        while (isspace((unsigned char)*p))
            p++;
        if (!*p)
            break;

        long long val = 0;
        int have_digit = 0;
        while (isdigit((unsigned char)*p))
        {
            have_digit = 1;
            val = val * 10 + (*p - '0');
            p++;
        }
        if (!have_digit)
            return -1;
  
        char unit = 's';
        if (*p && isalpha((unsigned char)*p))
        {
            unit = (char)tolower((unsigned char)*p);
            p++;
        }
        switch (unit)
        {
        case 'h':
            total += val * 3600LL;
            break;
        case 'm':
            total += val * 60LL;
            break;
        case 's':
            total += val;
            break;
        default:
            return -1;
        }
        while (*p && (isspace((unsigned char)*p) || *p == ','))
            p++;
    }
    return total > 0 ? total : -1;
}

static void format_hhmmss(long long secs, char *buf, size_t buflen)
{
    long long h = secs / 3600LL;
    long long m = (secs % 3600LL) / 60LL;
    long long s = secs % 60LL;
    if (h > 0)
        snprintf(buf, buflen, "%02lld:%02lld:%02lld", h, m, s);
    else
        snprintf(buf, buflen, "%02lld:%02lld", m, s);
}

static void ring_bell(int times)
{
    for (int i = 0; i < times; ++i)
    {
        fputs("\a", stdout);
        fflush(stdout);
        sleep_milliseconds(250);
    }
}

static void timestamp_now(char *buf, size_t buflen)
{
    time_t t = time(NULL);
    struct tm tm;
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    strftime(buf, buflen, "%Y-%m-%d %H:%M:%S", &tm);
}

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s [--once|-o] [--message|-m MESSAGE] DURATION\n"
            "\n"
            "DURATION formats:\n"
            "  25m, 1h30m, 45s, 1h5m20s, 15:00, 01:15:00\n\n"
            "Examples:\n"
            "  %s 25m\n"
            "  %s -m \"Time for coffee!\" 45m\n"
            "  %s --once 1h\n",
            prog, prog, prog, prog);
}

int main(int argc, char **argv)
{
    int repeat = 1;
    const char *message = "Time for coffee!";
    const char *duration_str = NULL;

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (strcmp(arg, "-o") == 0 || strcmp(arg, "--once") == 0)
        {
            repeat = 0;
        }
        else if (strcmp(arg, "-m") == 0 || strcmp(arg, "--message") == 0)
        {
            if (i + 1 >= argc)
            {
                print_usage(argv[0]);
                return 1;
            }
            message = argv[++i];
        }
        else if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
        {
            print_usage(argv[0]);
            return 0;
        }
        else if (arg[0] == '-')
        {
            fprintf(stderr, "Unknown option: %s\n", arg);
            print_usage(argv[0]);
            return 1;
        }
        else
        {
            duration_str = arg;
        }
    }

    if (!duration_str)
    {
        fprintf(stderr, "Missing DURATION.\n");
        print_usage(argv[0]);
        return 1;
    }

    long long seconds = parse_duration(duration_str);
    if (seconds <= 0)
    {
        fprintf(stderr, "Invalid DURATION: %s\n", duration_str);
        return 1;
    }

    install_signal_handlers();

    do
    {
        long long remaining = seconds;
        char buf[32];

        printf("Starting coffee timer: %s (%lld seconds)\n", duration_str, seconds);
        fflush(stdout);

        while (g_running && remaining >= 0) {
            format_hhmmss(remaining, buf, sizeof(buf));
            printf("\r⏳  %s remaining ", buf);
            fflush(stdout);
            if (remaining == 0)
                break;
            sleep_seconds(1);
            --remaining;
        }

        if (!g_running) break;

        char tbuf[64];
        timestamp_now(tbuf, sizeof tbuf);
        printf("\r✅  %s  (%s)\n", message, tbuf);
        fflush(stdout);
        ring_bell(3);

    } while (repeat && g_running);

    if (!g_running)
    {
        printf("\nCancelled. Bye!\n");
    }
    return 0;
}
