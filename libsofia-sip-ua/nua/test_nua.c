/*
 * This file is part of the Sofia-SIP package
 *
 * Copyright (C) 2005 Nokia Corporation.
 *
 * Contact: Pekka Pessi <pekka.pessi@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

/**@CFILE test_nua.c
 * @brief High-level tester for Sofia SIP User Agent Engine
 *
 * @author Pekka Pessi <Pekka.Pessi@nokia.com>
 * @author Martti Mela <Martti Mela@nokia.com>
 *
 * @date Created: Wed Aug 17 12:12:12 EEST 2005 ppessi
 */

#include "config.h"

#include "test_nua.h"

#if HAVE_ALARM
#include <signal.h>
#endif

#if defined(_WIN32)
#include <fcntl.h>
#endif

SOFIAPUBVAR su_log_t nua_log[];
SOFIAPUBVAR su_log_t soa_log[];
SOFIAPUBVAR su_log_t nea_log[];
SOFIAPUBVAR su_log_t nta_log[];
SOFIAPUBVAR su_log_t tport_log[];
SOFIAPUBVAR su_log_t su_log_default[];

char const name[] = "test_nua";
int print_headings = 1;
int tstflags = 0;

#if HAVE_FUNC
#elif HAVE_FUNCTION
#define __func__ __FUNCTION__
#else
#define __func__ name
#endif

#if HAVE_ALARM
static RETSIGTYPE sig_alarm(int s)
{
  fprintf(stderr, "%s: FAIL! test timeout!\n", name);
  exit(1);
}
#endif

static char const options_usage[] =
  "   -v | --verbose    be verbose\n"
  "   -q | --quiet      be quiet\n"
  "   -s                use only single thread\n"
  "   -l level          set logging level (0 by default)\n"
  "   -e | --events     print nua events\n"
  "   -A                print nua events for A\n"
  "   -B                print nua events for B\n"
  "   -C                print nua events for C\n"
  "   --attach          print pid, wait for a debugger to be attached\n"
  "   --no-proxy        do not use internal proxy\n"
  "   --no-nat          do not use internal \"nat\"\n"
  "   --symmetric       run internal \"nat\" in symmetric mode\n"
  "   -N                print events from internal \"nat\"\n"
  "   --no-alarm        don't ask for guard ALARM\n"
  "   -p uri            specify uri of outbound proxy (implies --no-proxy)\n"
  "   --proxy-tests     run tests involving proxy, too\n"
  "   -k                do not exit after first error\n"
  ;

void usage(int exitcode)
{
  fprintf(stderr, "usage: %s OPTIONS\n   where OPTIONS are\n%s",
	    name, options_usage);
  exit(exitcode);
}

int main(int argc, char *argv[])
{
  int retval = 0;
  int i, o_quiet = 0, o_attach = 0, o_alarm = 1;
  int o_events_init = 0, o_events_a = 0, o_events_b = 0, o_events_c = 0;
  int o_iproxy = 1, o_inat = 1;
  int o_inat_symmetric = 0, o_inat_logging = 0, o_expensive = 0;
  url_t const *o_proxy = NULL;
  int level = 0;

  struct context ctx[1] = {{{ SU_HOME_INIT(ctx) }}};

  if (getenv("EXPENSIVE_CHECKS"))
    o_expensive = 1;

  ctx->threading = 1;
  ctx->quit_on_single_failure = 1;

  endpoint_init(ctx, &ctx->a, 'a');
  endpoint_init(ctx, &ctx->b, 'b');
  endpoint_init(ctx, &ctx->c, 'c');

  for (i = 1; argv[i]; i++) {
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
      tstflags |= tst_verbatim;
    else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--abort") == 0)
      tstflags |= tst_abort;
    else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quiet") == 0)
      tstflags &= ~tst_verbatim, o_quiet = 1;
    else if (strcmp(argv[i], "-k") == 0)
      ctx->quit_on_single_failure = 0;
    else if (strncmp(argv[i], "-l", 2) == 0) {
      char *rest = NULL;

      if (argv[i][2])
	level = strtol(argv[i] + 2, &rest, 10);
      else if (argv[i + 1])
	level = strtol(argv[i + 1], &rest, 10), i++;
      else
	level = 3, rest = "";

      if (rest == NULL || *rest)
	usage(1);

      su_log_set_level(nua_log, level);
      su_log_soft_set_level(soa_log, level);
      su_log_soft_set_level(nea_log, level);
      su_log_soft_set_level(nta_log, level);
      su_log_soft_set_level(tport_log, level);
    }
    else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--events") == 0) {
      o_events_init = o_events_a = o_events_b = o_events_c = 1;
    }
    else if (strcmp(argv[i], "-I") == 0) {
      o_events_init = 1;
    }
    else if (strcmp(argv[i], "-A") == 0) {
      o_events_a = 1;
    }
    else if (strcmp(argv[i], "-B") == 0) {
      o_events_b = 1;
    }
    else if (strcmp(argv[i], "-C") == 0) {
      o_events_c = 1;
    }
    else if (strcmp(argv[i], "-s") == 0) {
      ctx->threading = 0;
    }
    else if (strcmp(argv[i], "--attach") == 0) {
      o_attach = 1;
    }
    else if (strncmp(argv[i], "-p", 2) == 0) {
      if (argv[i][2])
	o_proxy = URL_STRING_MAKE(argv[i] + 2)->us_url;
      else if (!argv[++i] || argv[i][0] == '-')
	usage(1);
      else
	o_proxy = URL_STRING_MAKE(argv[i])->us_url;
    }
    else if (strcmp(argv[i], "--proxy-tests") == 0) {
      ctx->proxy_tests = 1;
    }
    else if (strcmp(argv[i], "--no-proxy") == 0) {
      o_iproxy = 0;
    }
    else if (strcmp(argv[i], "--no-nat") == 0) {
      o_inat = 0;
    }
    else if (strcmp(argv[i], "--nat") == 0) {
      o_inat = 1;
    }
    else if (strcmp(argv[i], "--symmetric") == 0) {
      o_inat_symmetric = 1;
    }
    else if (strcmp(argv[i], "-N") == 0) {
      o_inat_logging = 1;
    }
    else if (strcmp(argv[i], "--expensive") == 0) {
      o_expensive = 1;
    }
    else if (strcmp(argv[i], "--no-alarm") == 0) {
      o_alarm = 0;
    }
#if SU_HAVE_OSX_CF_API /* If compiled with CoreFoundation events */
    else if (strcmp(argv[i], "--osx-runloop") == 0) {
      ctx->osx_runloop = 1;
    }
#endif
    else if (strcmp(argv[i], "-") == 0) {
      i++; break;
    }
    else if (argv[i][0] != '-') {
      break;
    }
    else
      usage(1);
  }

  if (o_attach) {
    char line[10], *l;
    printf("%s: pid %lu\n", name, (unsigned long)getpid());
    printf("<Press RETURN to continue>\n");
    l = fgets(line, sizeof line, stdin);
  }
#if HAVE_ALARM
  else if (o_alarm) {
    alarm(o_expensive ? 60 : 120);
    signal(SIGALRM, sig_alarm);
  }
#endif

  su_init();

  if (!(TSTFLAGS & tst_verbatim)) {
    if (level == 0 && !o_quiet)
      level = 1;
    su_log_soft_set_level(nua_log, level);
    su_log_soft_set_level(soa_log, level);
    su_log_soft_set_level(nea_log, level);
    su_log_soft_set_level(nta_log, level);
    su_log_soft_set_level(tport_log, level);
  }

  if (!o_quiet || (TSTFLAGS & tst_verbatim)
      || o_events_a || o_events_b || o_events_c)
    print_headings = 1;

#define SINGLE_FAILURE_CHECK()						\
  do { fflush(stdout);							\
    if (retval && ctx->quit_on_single_failure) {			\
      su_deinit(); return retval; }					\
  } while(0)

  ctx->a.printer = o_events_init ? print_event : NULL;

  retval |= test_nua_api_errors(ctx); SINGLE_FAILURE_CHECK();
  retval |= test_tag_filter(); SINGLE_FAILURE_CHECK();
  retval |= test_nua_params(ctx); SINGLE_FAILURE_CHECK();

  retval |= test_nua_init(ctx, o_iproxy, o_proxy, o_inat,
			  TESTNATTAG_SYMMETRIC(o_inat_symmetric),
			  TESTNATTAG_LOGGING(o_inat_logging),
			  TAG_END());

  ctx->expensive = o_expensive;

  if (retval == 0) {
    ctx->a.printer = o_events_a ? print_event : NULL;
    if (o_events_b)
      ctx->b.printer = print_event;
    if (o_events_c)
      ctx->c.printer = print_event;

    retval |= test_stack_errors(ctx); SINGLE_FAILURE_CHECK();

    retval |= test_register(ctx);

    if (retval == 0)
      retval |= test_connectivity(ctx);

    if (retval == 0 && o_inat)
      retval |= test_nat_timeout(ctx);

    if (retval == 0) {
      retval |= test_extension(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_basic_call(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_reject_a(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_reject_b(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_reject_302(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_reject_401(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_mime_negotiation(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_reject_401_aka(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_call_cancel(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_call_destroy(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_early_bye(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_reinvites(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_session_timer(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_refer(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_100rel(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_simple(ctx); SINGLE_FAILURE_CHECK();
      retval |= test_events(ctx); SINGLE_FAILURE_CHECK();
    }

    if (ctx->proxy_tests && (retval == 0 || !ctx->p))
      retval |= test_unregister(ctx); SINGLE_FAILURE_CHECK();
  }
  retval |= test_deinit(ctx);

  su_home_deinit(ctx->home);

  su_deinit();

  return retval;
}