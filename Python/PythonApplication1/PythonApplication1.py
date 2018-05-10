import re
import argparse
import os
import traceback

argregex = re.compile("%(s|d|u|i|(lu)|p|(ld)|x|f|c)")

def aux(str, tp):
  return str[tp[0]:tp[1]]

def format_args(str, format, gen):
    return re.sub(argregex,
                lambda s:"\" << " + aux(str, next(gen)) + " << \"",
                aux(str, format))

def extract_coma_separated(str, start):
  in_double_quote = False
  parenthesis_level = 0
  idx = start
  while (str[idx] != ',' and str[idx] != ')') or in_double_quote or parenthesis_level > 0:
    #escape char
    if str[idx] == '\\':
        idx += 1
    if str[idx] == '"':
      in_double_quote = not in_double_quote
    if str[idx] == '(' and not in_double_quote:
      parenthesis_level += 1
    if str[idx] == ')' and not in_double_quote:
      parenthesis_level -= 1
    idx += 1
  return idx

def find_debug(str, beginning):
  begin_arg_idx = beginning + 9
  arg_idx = begin_arg_idx
  while str[arg_idx] != ')':
    begin_arg_idx = arg_idx + 1
    arg_idx = extract_coma_separated(str, begin_arg_idx)
    yield(begin_arg_idx, arg_idx)
  yield (beginning, arg_idx)

def replace_generic(retuning_index_lambda, str):
  start = 0
  while len(str) > start:
    try:
      start = retuning_index_lambda(str, start)
    except ValueError:
      return str
    gen = find_debug(str, start)
    log_level = aux(str, next(gen))
    fmt = format_args(str, next(gen), gen)
    if log_level in set(["ERROR", "WARNING", "INFO", "FATAL"]):
      new_dbg = "LOG(" + log_level + ") << " + fmt
    else:
      new_dbg = "LOG(INFO) << \"" + log_level + ":\" << " + fmt
    (b, e) = next(gen)
    start = e
    try:
      next(gen)
    except StopIteration:
      str = str[:b] + new_dbg + str[e + 1:]
      continue
    print("Error when trying to substitute " + new_dbg)
  return str

def replace_debug(str):
  return replace_generic(lambda s, start: s.index(r"debug(LOG_", start), str)

def replace_assert(match):
  return replace_generic(lambda s, start: s.index(r"ASSERT(", start), str)

def force_open(file):
  try:
    f = open(file, 'r', encoding = "utf-8")
    text = f.read()
    f.close()
    return text
  except UnicodeDecodeError:
    f = open(os.path.join(root, file), 'r', encoding = "CP1252")
    text = f.read()
    f.close()
    return text

def process_dir(dir):
  for root, dirs, files in os.walk(dir):
    for file in files:
      if file.endswith(".cpp"):
        text = force_open(os.path.join(root, file))
        try:
          changed_text = replace_debug(text)
          fileN = open(os.path.join(root, file), "w", encoding = "utf-8")
          fileN.write(changed_text)
          fileN.close()
        except:
          print("Error occured when processing " + file)
          traceback.print_exc()

if __name__ == '__main__':
  for d in ["../../src/", "../../lib/"]:
    process_dir(d)
