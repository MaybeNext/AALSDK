// INTEL CONFIDENTIAL - For Intel Internal Use Only

#define MINUTES_IN_TERMS_OF_MILLIS(__x) ( ((AAL::btTime)__x) * ((AAL::btTime)60000) )
#define HOURS_IN_TERMS_OF_MILLIS(__x)   ( ((AAL::btTime)__x) * ((AAL::btTime)60000) * ((AAL::btTime)60) )

#if   defined( __AAL_WINDOWS__ )
# define cpu_yield()       ::Sleep(0)
# define sleep_millis(__x) ::Sleep(__x)
#elif defined( __AAL_LINUX__ )
# define cpu_yield()       ::usleep(0)
# define sleep_millis(__x) ::usleep((__x) * 1000)
#endif // OS

class GlobalTestConfig
{
public:
   GlobalTestConfig() {}

   // Certain thread tests require entering a tight loop, yielding the cpu in order
   // to allow other threads to reach some state. Defines the max number of polls
   // for such loops.
   AAL::btUIntPtr MaxCPUYieldPolls() const { return 100; }

} Config;

// Enter a tight loop, yielding the cpu so long as __predicate evaluates to true.
#define YIELD_WHILE(__predicate) \
do                               \
{                                \
   while ( __predicate ) {       \
      cpu_yield();               \
   }                             \
}while(0)

// Yield the cpu Config.MaxCPUYieldPolls() times.
#define YIELD_N()                                        \
do                                                       \
{                                                        \
   AAL::btUIntPtr       __i;                             \
   const AAL::btUIntPtr __N = Config.MaxCPUYieldPolls(); \
   for ( __i = 0 ; __i < __N ; ++__i ) {                 \
      cpu_yield();                                       \
   }                                                     \
}while(0)

// Yield the cpu Config.MaxCPUYieldPolls() times, executing __expr after each yield.
#define YIELD_N_FOREACH(__expr)                          \
do                                                       \
{                                                        \
   AAL::btUIntPtr       __i;                             \
   const AAL::btUIntPtr __N = Config.MaxCPUYieldPolls(); \
   for ( __i = 0 ; __i < __N ; ++__i ) {                 \
      cpu_yield();                                       \
      __expr ;                                           \
   }                                                     \
}while(0)

////////////////////////////////////////////////////////////////////////////////

#define ASSERT_NONNULL(x) ASSERT_NE((void *)NULL, x)
#define ASSERT_NULL(x)    ASSERT_EQ((void *)NULL, x)
#define EXPECT_NONNULL(x) EXPECT_NE((void *)NULL, x)
#define EXPECT_NULL(x)    EXPECT_EQ((void *)NULL, x)

const std::string SampleAFU1ConfigRecord("9 20 ConfigRecordIncluded\n \
      \t10\n \
          \t\t9 17 ServiceExecutable\n \
            \t\t\t9 13 libsampleafu1\n \
         \t\t9 18 _CreateSoftService\n \
         \t\t0 1\n \
   9 29 ---- End of embedded NVS ----\n \
      9999\n");

// Retrieve the current test and test case name from gtest.
// Must be called within the context of a test case/fixture.
void TestCaseName(std::string &Test, std::string &TestCase)
{
   const ::testing::TestInfo * const pInfo =
      ::testing::UnitTest::GetInstance()->current_test_info();

   Test     = std::string(pInfo->name());
   TestCase = std::string(pInfo->test_case_name());
}

#if defined( __AAL_LINUX__ )

// Make sure that the given path appears in LD_LIBRARY_PATH, preventing duplication.
// Return non-zero on error.
int RequireLD_LIBRARY_PATH(const char *path)
{
   int   res  = 1;
   char *pvar = getenv("LD_LIBRARY_PATH");

   if ( NULL == pvar ) {
      // not found, so set it.
      return setenv("LD_LIBRARY_PATH", path, 1);
   }

   char *pcopyvar  = strdup(pvar);
   char *psavecopy = pcopyvar;

   if ( NULL == pcopyvar ) {
      return res;
   }

   char *pcolon;
   while ( NULL != (pcolon = strchr(pcopyvar, ':')) ) {

      *pcolon = 0;

      if ( 0 == strcmp(pcopyvar, path) ) {
         // path already found in LD_LIBRARY_PATH
         res = 0;
         goto _DONE;
      }

      pcopyvar = pcolon + 1;

   }

   if ( 0 == strcmp(pcopyvar, path) ) {
      // path already found in LD_LIBRARY_PATH
      res = 0;
      goto _DONE;
   }

   // LD_LIBRARY_PATH exists, but does not contain path.

   free(psavecopy);

   if ( 0 == strcmp(pvar, "") ) {
      // LD_LIBRARY_PATH is defined, but empty.
      return setenv("LD_LIBRARY_PATH", path, 1);
   }

   psavecopy = (char *) malloc(strlen(pvar) + strlen(path) + 2);
   if ( NULL == psavecopy ) {
      return res;
   }

   sprintf(psavecopy, "%s:%s", pvar, path);

   res = setenv("LD_LIBRARY_PATH", psavecopy, 1);

_DONE:
   free(psavecopy);

   return res;
}

// Print streamer for LD_LIBRARY_PATH.
// Ex.
//   cout << LD_LIBRARY_PATH << endl;
std::ostream & LD_LIBRARY_PATH(std::ostream &os)
{
   char *pvar = getenv("LD_LIBRARY_PATH");

   if ( NULL != pvar ) {
      os << pvar;
   }

   return os;
}

#endif // __AAL_LINUX__


#if   defined( __AAL_WINDOWS__ )
# error TODO implement SignalHelper class for windows.
#elif defined( __AAL_LINUX__ )
# include <errno.h>
# include <unistd.h>
# include <sys/types.h>
# include <signal.h>

class SignalHelper
{
public:
   SignalHelper() {}
   virtual ~SignalHelper();

   typedef void (*handler)(int , siginfo_t * , void * );

   // Does not allow hooking the same signum multiple times.
   // non-zero on error.
   int Install(int signum, handler h, bool oneshot=false);

   static void   EmptySIGIOHandler(int , siginfo_t * , void * );
   static void EmptySIGUSR1Handler(int , siginfo_t * , void * );
   static void EmptySIGUSR2Handler(int , siginfo_t * , void * );

protected:
   typedef std::map<int, struct sigaction> sigmap;
   typedef sigmap::iterator                sigiter;
   typedef sigmap::const_iterator          const_sigiter;

   sigmap m_sigmap;
};

SignalHelper::~SignalHelper()
{
   // re-map each signal to its original handler.
   const_sigiter iter;
   for ( iter = m_sigmap.begin() ; m_sigmap.end() != iter ; ++iter ) {
      ::sigaction(iter->first, &iter->second, NULL);
   }

}

int SignalHelper::Install(int signum, handler h, bool oneshot)
{
   if ( NULL == h ) {
      return -1;
   }

   struct sigaction act;
   memset(&act, 0, sizeof(act));

   act.sa_flags     = SA_SIGINFO;
   if ( oneshot ) {
      act.sa_flags |= SA_RESETHAND;
   }
   act.sa_sigaction = h;

   struct sigaction orig;
   memset(&orig, 0, sizeof(orig));

   int res = ::sigaction(signum, &act, &orig);

   if ( 0 != res ) {
      return res;
   }

   std::pair<sigiter, bool> ins = m_sigmap.insert(std::make_pair(signum, orig));

   return ins.second ? res : -2;
}

void SignalHelper::EmptySIGIOHandler(int sig, siginfo_t *info, void * /* unused */)
{
   EXPECT_EQ(SIGIO,    sig);
   EXPECT_EQ(SIGIO,    info->si_signo);
   EXPECT_EQ(SI_TKILL, info->si_code);
}

void SignalHelper::EmptySIGUSR1Handler(int sig, siginfo_t *info, void * /* unused */)
{
   EXPECT_EQ(SIGUSR1,  sig);
   EXPECT_EQ(SIGUSR1,  info->si_signo);
   EXPECT_EQ(SI_TKILL, info->si_code);
}

void SignalHelper::EmptySIGUSR2Handler(int sig, siginfo_t *info, void * /* unused */)
{
   EXPECT_EQ(SIGUSR2,  sig);
   EXPECT_EQ(SIGUSR2,  info->si_signo);
   EXPECT_EQ(SI_TKILL, info->si_code);
}

#endif // OS

