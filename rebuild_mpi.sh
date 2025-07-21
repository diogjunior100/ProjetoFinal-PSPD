docker build -t pspd_mpi1:latest OMP_MPI
minikube image load pspd_mpi1:latest
kubectl apply -f kubernetes/mpi.yaml