import os
from pathlib import Path

LOGLEVEL_MAP = {
  'none': (0, 'LOG_NONE'),
  'log_none': (0, 'LOG_NONE'),
  'error': (1, 'LOG_ERROR'),
  'log_error': (1, 'LOG_ERROR'),
  'warn': (2, 'LOG_WARN'),
  'log_warn': (2, 'LOG_WARN'),
  'warning': (2, 'LOG_WARN'),
  'log_warning': (2, 'LOG_WARN'),
  'info': (3, 'LOG_INFO'),
  'log_info': (3, 'LOG_INFO'),
  'debug': (4, 'LOG_DEBUG'),
  'log_debug': (4, 'LOG_DEBUG'),
  'verbose': (5, 'LOG_VERBOSE'),
  'log_verbose': (5, 'LOG_VERBOSE')
}

def __bool(key: str, value: str) -> bool:
  if value == None:
    return False

  value = value.lower()

  if value == 'true' or value == '1':
    return True
  elif value == 'false' or value == '0':
    return False
  else:
    raise ValueError('Environment variable ' + key + ' is not a boolean value.')

def get_bool(key: str) -> bool:
  return __bool(key, os.environ.get(key, None))

def get_github_ref_name():
  return os.environ.get('GITHUB_REF_NAME', None)

def get_github_base_ref():
  return os.environ.get('GITHUB_BASE_REF', None)

def get_github_event_name():
  return os.environ.get('GITHUB_EVENT_NAME', None)

def is_github_ci():
  return bool('CI') and bool('GITHUB_ACTIONS')

def is_github_pr():
  return get_github_event_name() == 'pull_request'

class dotenv:
  def __read_dotenv(self, path: str):
    with open(path) as f:
      for line in f:
        line = line.strip()
        if line == '' or line.startswith('#'):
            continue

        key, value = line.strip().split('=', 1)

        self.dotenv_vars[key] = value

  def __init__(self, path: str | Path, environment: str):
    self.dotenv_vars: dict[str, str] = {}

    if isinstance(path, str):
      path = Path(path)

    # Read .env file(s) into environment variables, start from the root of the drive and work our way down.
    paths = list(path.parents[::-1]) + [path]

    env_specific_name = '.env.' + environment

    # Read the .env files.
    for path in paths:
      env_file = path / '.env'
      if env_file.exists():
        self.__read_dotenv(env_file)

      env_file = path / env_specific_name
      if env_file.exists():
        self.__read_dotenv(env_file)

      env_file = path / '.env.local'
      if env_file.exists():
        self.__read_dotenv(env_file)

  def get_str(self, key: str, dotenv: bool = True):
    return self.dotenv_vars.get(key, os.environ.get(key, None))

  def get_all_prefixed(self, prefix: str, dotenv: bool = True):
    result: dict[str, str] = {}
    for key, value in os.environ.items():
      if key.startswith(prefix):
        result[key] = value

    if dotenv:
      for key, value in self.dotenv_vars.items():
        if key.startswith(prefix):
          result[key] = value

    return result

  def is_equals(self, key: str, value: str, dotenv: bool = True):
    return self.get_str(key, dotenv) == value

  def get_bool(self, key: str, dotenv: bool = True):
    return __bool(key, self.get_str(key, dotenv))

  def get_int(self, key: str, dotenv: bool = True):
    return int(self.get_str(key, dotenv))

  def get_loglevel(self, key: str, dotenv: bool = True) -> tuple[int, str]:
    value = self.get_str(key, dotenv)

    value = value.lower()

    tup = LOGLEVEL_MAP.get(value, None)
    if tup == None:
      raise ValueError('Environment variable ' + key + ' (' + value + ') is not a valid log level.')

    return tup
