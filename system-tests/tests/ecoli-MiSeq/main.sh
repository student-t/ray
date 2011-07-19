

mpirun -tag-output -np $NSLOTS $RAY_GIT_PATH/code/Ray  \
-p MiSeq_Ecoli_MG1655_110527_R1.fastq \
   MiSeq_Ecoli_MG1655_110527_R2.fastq \
-o $TEST_NAME -show-memory-usage -k 31 -show-extension-choice

ValidateGenomeAssembly.sh Ecoli-k12-mg1655.fasta $TEST_NAME.Contigs.fasta $TEST_NAME.Ray
