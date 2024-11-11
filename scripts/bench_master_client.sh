# # IKNP OHE
# printf "IKNP OHE -\n"
# ./bench_ohe_batched.sh 2 10000 $1 iknp ohe 128 2 16

# # IKNP GMT
# printf "\nIKNP GMT -\n"
# ./bench_ohe_batched.sh 2 10000 $1 iknp gmt 128 2 16

# # Ferret OHE
# printf "\nFerret OHE -\n"
# ./bench_ohe_batched.sh 2 10000 $1 ferret ohe 4096 2 16

# # Ferret GMT
# printf "\nFerret GMT -\n"
# ./bench_ohe_batched.sh 2 10000 $1 ferret gmt 4096 2 16

# # Online 128
# printf "\nOnline 128 -\n"
# ./bench_lut_batched.sh 2 10000 $1 iknp ohe 128 2 16

# # Online 4096
# printf "\nOnline 4096 -\n"
# ./bench_lut_batched.sh 2 10000 $1 iknp ohe 4096 2 16

# Budget Time GMT
printf "\nBudget Time GMT -\n"
./bench_ferret_budget_time_gmt.sh 2 10000 $1

# GMT-Batches Time OHE
printf "\nGMT-Batches Time OHE -\n"
./bench_ferret_gmt-batches_time_ohe.sh 2 10000 $1

# Budget Time OHE
printf "\nBudget Time OHE -\n"
./bench_ferret_budget_time_ohe.sh 2 10000 $1

# OHE-Batches Time GMT
printf "\nOHE-Batches Time GMT -\n"
./bench_ferret_ohe-batches_time_gmt.sh 2 10000 $1