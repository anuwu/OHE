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
    ohe_len = int(sys.argv[1])
    tool = int(sys.argv[2])
    tool_string = "ohe" if tool == 1 else "gmt"

    dic = {}
    dic["Lengths"] = [str(2**x) for x in range(2, 3)]
    
    for parties in range(7, 9) :
        print(f"{parties} parties -")
        # for ot in ["simple", "iknp"] :
        for ot in ["ferret"] :
            col_time, col_comm = [], []

            base_call_string = f"./build/ppohe 1 {parties} 10000 {ohe_len} {ot} {tool} > bench/ppohe_{ot}_{tool_string}_1_{parties}.txt"
            call_string = base_call_string
            for i in range(2, parties+1) :
                call_string = call_string + f" & (./build/ppohe {i} {parties} 10000 {ohe_len} {ot} {tool} > bench/ppohe_{ot}_{tool_string}_{i}_{parties}.txt)"

            # print(call_string)
            st = str(subprocess.check_output(["zsh", "-c", call_string]))
            print(f"\t{ot} done")
    #     time, comm = extract(str(st))
    #     col_time.append(time)
    #     col_comm.append(comm)
    #     print(f"\t{length} done")

    #     dic[ot + "_time"] = col_time
    #     dic[ot + "_comm"] = col_comm
    #     print(f"{ot} done")
    
    # df = pd.DataFrame(dic)
    # tool_string = "ohe" if tool == 1 else "gmt"
    # df.to_csv(f"ppohe_{tool_string}_{party}_{parties}.csv", index=False)
