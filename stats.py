"""
This program is just to check the accuracy of the program
we wrote in C to compute the average and max of all non-negative
numbers in the file keys.txt.  This program reads the file keys.txt
and computes the average and max of all non-negative numbers in the file.
"""

# Compute the average and max of all non-negative numbers in the file keys.txt
# and print them out.

# Open the file keys.txt for reading
f = open("input_value_file_pt1_b.txt", "r")

# Initialize the sum and count variables
sum = 0

# Read the first line from the file
line = f.readline()

# Loop through the file until the end of the file is reached
count = 0
maxNum = None
while line != "":
    # Convert the line to an integer
    num = int(line)
    if(maxNum == None):
        maxNum = num

    # Add the number to the sum if the number is non-negative
    if num >= 0:
      sum += num
      count += 1
      maxNum = max(maxNum, num)

    # Read the next line from the file
    line = f.readline()


# Close the file
f.close()

# Compute the average
average = sum / count

# Print the average and max
print ("Average:", average)
print ("Max:", maxNum)