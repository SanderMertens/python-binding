/* $CORTO_GENERATED
 *
 * test_Project.c
 *
 * Only code written between the begin and end tags will be preserved
 * when the file is regenerated.
 */

#include "test.h"

/* $header() */

static const char* scriptHeader = 
    "import os\n"
    "import sys\n"
    "corto_path = os.path.realpath(os.path.join(os.getcwd(), \"..\"))\n"
    "sys.path.append(corto_path)\n"
    "import corto\n"
    "import unittest\n"
;

/* $end */

corto_void _test_Project_tc_root_o(test_Project this) {
/* $begin(test/Project/tc_root_o) */
    _test_Project_testScript(
        this,
        "class Test(unittest.TestCase):\n"
        "    def setUp(self):\n"
        "        corto.start()\n"
        "    def tearDown(self):\n"
        "        corto.stop()\n"
        "    def test_resolve_lang(self):\n"
        "        self.assertIsNotNone(corto.ROOT_O)\n"
    );
/* $end */
}

corto_void _test_Project_tc_startStop(test_Project this) {
/* $begin(test/Project/tc_startStop) */
    _test_Project_testScript(
        this,
        "class Test(unittest.TestCase):\n"
        "    def setUp(self):\n"
        "        corto.start()\n"
        "    def tearDown(self):\n"
        "        corto.stop()\n"
        "    def test_root(self):\n"
        "        pass\n"
    );
/* $end */
}

corto_int16 _test_Project_testScript(test_Project this, corto_string testCase) {
/* $begin(test/Project/testScript) */
    FILE* scriptFile = fopen("./python-binding-test.py", "w");
    if (!scriptFile) {
        corto_seterr("could not create python-binding-test");
        goto error;
    }
    fprintf(scriptFile, scriptHeader);
    fprintf(scriptFile, testCase);
    if (fclose(scriptFile)) {
        corto_seterr("could not close scriptFile");
        goto errorRemoveScript;
    }

    corto_pid pid = corto_procrun("python3", (char*[]){"python3", "-m", "unittest", "python-binding-test", NULL});
    corto_int8 procResult = 0;
    corto_procwait(pid, &procResult);
    printf("procresult: %d\n", procResult);
    test_assert(procResult == 0);
    corto_rm("./python-binding-test.py");
    return 0;
errorRemoveScript:
    corto_rm("./python-binding-test.py");
error:
    return -1;
/* $end */
}
