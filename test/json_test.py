import os
import subprocess
import time
TEST_DATA_DIR = ['./JSONTestSuite/test_parsing']

def test():
      for dir in TEST_DATA_DIR:
            total_test = 0
            passed = 0
            not_passed = 0
            list_files = os.listdir(dir)
            dump_file_name = 'dump' + str(int(time.time()))
            dump_content = []
            for file in list_files:
                  full_path = os.path.join(dir, file)
                  ret = subprocess.run(['./json_test', full_path], stdout=subprocess.DEVNULL)
                  if ret.returncode == 0:
                        '''Test Passed'''
                        if file[0] == 'y':
                              '''Should be accepted'''
                              print("Testsuit name: %s, expected: Passed, got: \033[1;32mPassed\033[0m" % (full_path))
                              passed += 1
                        elif file[0] == 'n':
                              '''Should be rejected'''
                              print("Testsuit name: %s, expected: Not Passed, got: \033[1;31mPassed\033[0m" % (full_path))
                              dump_content.append(full_path)
                              not_passed += 1
                        else:
                              '''Free to accept or reject'''
                              print("Testsuit name: %s, expected: All, got: \033[0;33mPassed\033[0m" % (full_path))
                              passed += 1
                  else:
                        '''Test Not Passed'''
                        if file[0] == 'y':
                              '''Should be accepted'''
                              print("Testsuit name: %s, expected: Passed, got: \033[1;31mNot Passed\033[0m" % (full_path))
                              dump_content.append(full_path)
                              not_passed += 1
                        elif file[0] == 'n':
                              '''Should be rejected'''
                              print("Testsuit name: %s, expected: Not Passed, got: \033[1;32mNot Passed\033[0m" % (full_path))
                              passed += 1
                        else:
                              '''Free to accept or reject'''
                              print("Testsuit name: %s, expected: All, got: \033[0;33mNot Passed\033[0m" % (full_path))
                              passed += 1
                  
                  total_test += 1
            print("Total testsuits: %d Passed: %d Not Passed: %d" % (total_test, passed, not_passed))
            with open(dump_file_name, 'w') as f:
                  f.write('\n'.join(dump_content))
      


if __name__ == '__main__':
      test()
    