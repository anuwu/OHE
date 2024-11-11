# IKNP OHE
printf "IKNP OHE -\n"
./bench_ohe_batched.sh 1 10000 $1 iknp ohe 128 2 16 > iknp_ohe.txt

# IKNP GMT
printf "\nIKNP GMT -\n"
./bench_ohe_batched.sh 1 10000 $1 iknp gmt 128 2 16 > iknp_gmt.txt

# Ferret OHE
printf "\nFerret OHE -\n"
./bench_ohe_batched.sh 1 10000 $1 ferret ohe 4096 2 16 > ferret_ohe.txt

# Ferret GMT
printf "\nFerret GMT -\n"
./bench_ohe_batched.sh 1 10000 $1 ferret gmt 4096 2 16 > ferret_gmt.txt

# Online 128
printf "\nOnline 128 -\n"
./bench_lut_batched.sh 1 10000 $1 iknp ohe 128 2 16 > lut_128.txt

# Online 4096
printf "\nOnline 4096 -\n"
./bench_lut_batched.sh 1 10000 $1 iknp ohe 4096 2 16 > lut_4096.txt

# Budget Time OHE
printf "\nBudget Time OHE -\n"
./bench_ferret_budget_time_ohe.sh 1 10000 $1 > budget_time_ohe.txt

# Budget Time OHE
printf "\nBudget Time GMT -\n"
./bench_ferret_budget_time_ohe.sh 1 10000 $1 > budget_time_ohe.txt

# GMT-Batches Time OHE
printf "\nBudget Time OHE -\n"
./bench_ferret_budget_time_ohe.sh 1 10000 $1 > gmt-batches_time_ohe.txt

# OHE-Batches Time GMT
printf "\nBudget Time GMT -\n"
./bench_ferret_budget_time_ohe.sh 1 10000 $1 > ohe-batches_time_gmt.txt