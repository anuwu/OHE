#!/usr/python
import subprocess
import sys
import pandas as pd 

def extract(st) :
    taken_ind = st.find("taken")
    comm_ind = st.find("Comm")
    bytes_ind = st.find("bytes")

    time = st[taken_ind+8 : comm_ind-5]
    comm = st[comm_ind+7 : bytes_ind-1]
    return time, comm

if __name__ == "__main__" :
    party = sys.argv[1]

    dic = {}
    dic["Lengths"] = [str(2**x) for x in range(2, 17)]
    for ot in ["simple", "iknp", "otnp", "ferret"] :
        col_time, col_comm = [], []
        for length in range(2, 17) :
            call_string = f"./build/test {party} 12345 {length} {ot} 0"
            st = str(subprocess.check_output(["zsh", "-c", call_string]))
            time, comm = extract(str(st))
            col_time.append(time)
            col_comm.append(comm)

        dic[ot + "_time"] = col_time
        dic[ot + "_comm"] = col_comm
        print(f"{ot} done")
    
    df = pd.DataFrame(dic)
    df.to_csv(f"data_{party}.csv", index=False)
