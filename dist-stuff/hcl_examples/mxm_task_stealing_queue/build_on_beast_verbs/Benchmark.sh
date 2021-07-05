source setup.sh
echo "1 Rank\n" > RankTest.txt 
mpirun -n 3 -ppn 1 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 3 -ppn 1 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 3 -ppn 1 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
echo "2 Ranks" >> RankTest.txt 
mpirun -n 5 -ppn 2 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 5 -ppn 2 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 5 -ppn 2 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
echo "4 Ranks" >> RankTest.txt 
mpirun -n 9 -ppn 4 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 9 -ppn 4 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 9 -ppn 4 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
echo "8 Ranks" >> RankTest.txt 
mpirun -n 17 -ppn 8 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 17 -ppn 8 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 17 -ppn 8 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
echo "16 Ranks" >> RankTest.txt 
mpirun -n 33 -ppn 16 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 33 -ppn 16 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 33 -ppn 16 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
echo "32 Ranks" >> RankTest.txt 
mpirun -n 65 -ppn 32 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 65 -ppn 32 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
mpirun -n 65 -ppn 32 --host 10.12.1.2,10.12.1.1 ./main 2400 2 5 >> RankTest.txt
