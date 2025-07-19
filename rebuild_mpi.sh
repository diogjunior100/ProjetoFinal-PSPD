docker build -t pspd_mpi9:latest OMP_MPI
minikube image load pspd_mpi9:latest
kubectl apply -f kubernetes/mpi.yaml