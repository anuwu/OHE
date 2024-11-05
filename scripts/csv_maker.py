import pandas as pd
import sys

## Taken from https://medium.com/@Doug-Creates/convert-string-to-float-or-int-in-python-0d93ce89d7b1
def parse_string_to_number(input_string) :
  try :
    # Attempt to convert the string to an int
    return int(input_string)
  except ValueError :
    # If int conversion fails, attempt to convert to float
    try :
      return float(input_string)
    except ValueError :
      # If both conversions fail, return an error message
      return 'Error: Cannot convert to int or float'

# Function for parsing line with n
def get_n_from_line(ln) :
  index1 = ln.find('=') + 2
  index2 = ln.find('---') - 1
  return int(ln[index1:index2])

# Function for parsing line with attributes
def get_attr_from_line(ln) :
  colon_index = ln.find(':')
  label = ln[:colon_index-1]
  rem = ln[colon_index+2:]
  space_index = rem.find(' ')
  val = rem[:space_index]
  unit = rem[space_index+1:]
  return label, unit, parse_string_to_number(val)

# Method to get dataframe
def get_dataframe(filename) :
  # Open file
  file = open(filename, "r")
  text = file.read()
  lines = text.split('\n')
  no_lines = len(lines)

  # Attributes for first n
  i = 1
  col_names = ['n']
  col_types = ['int']
  data = [[get_n_from_line(lines[0])]]
  while lines[i].find("---") == -1 :
    label, unit, val = get_attr_from_line(lines[i])
    col_names = col_names + [f"{label} ({unit})"]
    data = data + [[val]]
    col_type = 'float' if unit == 'ms' else 'int'
    col_types.append(col_type)
    i += 1
  no_attrs = i-1

  # Attributes for remaining n
  while i <= no_lines :
    if lines[i] == '' :
      break
    n = get_n_from_line(lines[i])
    data[0].append(parse_string_to_number(n))
    for j in range(1, no_attrs+1) :
      _, _, val = get_attr_from_line(lines[i+j])
      data[j].append(val)
    i += no_attrs+1

  # Create dataframe
  df = pd.DataFrame(data, index=col_names).T
  for c, t in zip(col_names, col_types) :
      df[c] = df[c].astype(t)

  file.close()
  return df

if __name__ == "__main__" :
  no_args = len(sys.argv)
  if no_args != 3 :
    if no_args < 2 :
      print("Correct usage - ")
      print("python " + sys.argv[0] + " <input> <output>")
    elif no_args == 2 :
      if sys.argv[1] != "--help" :
        print("Correct usage - ")
      print("python " + sys.argv[0] + " <input> <output>")
    else :
      print("Correct usage - ")
      print("python " + sys.argv[0] + " <input> <output>")
    exit(0)

  input_filename = sys.argv[1]
  output_filename = sys.argv[2]
  df = get_dataframe(input_filename)
  df.to_csv(output_filename, index=False)