import unittest
import re
import string
import PythonApplication1

class TestFailure(unittest.TestCase):
  def test_check_doublequotes(self):
    str = r'debug(LOG_WARNING, "fileName is %s", (fileName == nullptr) ? "a NULL pointer" : "empty")'
    res = PythonApplication1.replace_debug(str)
    self.assertNotEqual(str, res)

  def test_check_doublequotes_inside_fmt(self):
    str = r'debug(LOG_FATAL, "Duplicate string for id: \"%s\"", "pID");'
    res = PythonApplication1.replace_debug(str)
    self.assertNotEqual(str, res)

  def test_check_coma(self):
    str = r'debug(LOG_ERROR, "Couldnt initialize audio context: %s", alcGetString(device, err))'
    res = PythonApplication1.replace_debug(str)
    self.assertNotEqual(str, res)

  def test_failing(self):
    str = r'debug(LOG_ERROR, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, WZ_PHYSFS_getLastError());'
    res = PythonApplication1.replace_debug(str)
    self.assertEqual(res.find("%s"), -1)
    self.assertNotEqual(str, res)

  #def test_check_no_param(self):
  #  str = r'debug(LOG_FATAL, "Unimplemented curve ID");'
  #  res = re.sub(PythonApplication1.log_regex, PythonApplication1.replace_match, str)
  #  self.assertNotEqual(str, res)

if __name__ == '__main__':
    unittest.main()
