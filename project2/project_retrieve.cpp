#include <stdio.h>
#include <stdlib.h>
#include <mpi.h> 
#include <stddef.h> 
#include <iostream>
#include <fstream>
#include <bits/stdc++.h>
#include <vector>
#include <time.h>
#include <queue>

using namespace std;

#define TAG 0 


#define SERVER_EVENT -1
#define ACK -2
#define START_LEADER_ELECTION -3
#define CANDIDATE_ID -4
#define CONNECT -5
#define LEADER_ELECTION_DONE -6
#define UPLOAD -7
#define LEAVE -8
#define UPLOAD_FAILED -9
#define UPLOAD_ACK -10
#define UPLOAD_OK -11
#define RETRIEVE -12
#define RETRIEVE_FAILED -12
#define RETRIEVE_ACK -13
#define RETRIEVE_OK -14
#define REQUEST_SHUTDOWN -15
#define INF_COORD -16

//helper functions-----------------------------------------------------------------
vector<vector<string>>  ReadInputFile(string name);
void init_msg(int dummy_int[3] , int elem0 , int elem1 , int elem2);
int contains_rank_v( vector<int> vect , int rank);
int contains_rank_l(list<int> l , int rank);
vector<int> select_processes(int N , list<int> l,int rank );
//---------------------------------------------------------------------------------

struct file{
	int fd;
	int version;
	file(int fd , int version){
		this->fd=fd;
		this->version=version;
	}
};

struct Client{
	vector<file> uploadedfiles;
	int active_requests;
	int leader_id;
	bool shutdown;
	Client(int leader_id){
		this->active_requests=0;
		this->leader_id=leader_id;
		this->shutdown=false;
	}
};

struct req_entry{
	int client_id;
	int counter;
	int type;
	int version;
	bool isrunning;
	req_entry(int client_id , int counter , int type , int version){
		this->client_id=client_id;
		this->counter=counter;
		this->type=type;
		this->version=version;
		this->isrunning=false;
	}
};

struct h_entry{
	file *f;
	queue<req_entry*> requests;
	h_entry(int f_id,int version){
		f = new file(f_id,version);
	}
};
struct Leader{
	vector<int> connections;
	map<int,h_entry*> htable;
};
struct Server{
	int rank;
	int l_neighbor;
	int r_neighbor;
	int leader_id;
	bool isconnected;
	bool shutdown;

	Leader* ld;
	
	int serversnum;
	list<int> servers_list;
	vector<file*> files;

	Server(int rank,int l_neighbor,int r_neighbor,int leader_id){
		this->shutdown=false;
		this->rank=rank;
		this->l_neighbor=l_neighbor;
		this->r_neighbor=r_neighbor;
		this->leader_id=leader_id;
		this->ld=NULL;
		this->serversnum=0;
		this->isconnected=false;
	}	

	int next_hop(int dest){
		if(rank==leader_id){
			int index = contains_rank_v(this->ld->connections,dest);
			if(index!=-1){
				//cout<<"here1";
				return dest;
			}else{
				index = contains_rank_l(this->servers_list,dest);
				while(index>0){
					int tmp = *next(this->servers_list.begin(),index);
					if(tmp==rank){return this->l_neighbor;}
					if(contains_rank_v(this->ld->connections,tmp)!=-1){
						//cout<<"here2 ";
						//if(dest==2){cout<<"CUPLOAD next hop: "<<tmp<<"\n";}
						return tmp;
					}
					++index;
				}
			}
		}else if(dest==leader_id && isconnected){
			return leader_id;
		}
		//cout<<"here3 ";
		return this->l_neighbor;
	}
};

class Coordinator{
	private:
	int index;
	public:
	int leader;
	vector<vector<string>> input;
	vector<int> servers;
	vector<int> clients;
	Coordinator(vector<vector<string>> input){
		this->input=input;
		index = 0;
	}
	vector<string> getnext(){
		++index;
		return input.at(index-1);
	}
	vector<string> get(){
		return input.at(index-1);
	}
	int getindex(){
		return index;
	}

};
int main(int argc, char *argv[]){

	int rank, world_size;
	/** MPI Initialisation **/
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Status status;
	//MPI_Datatype ;
	
	/** Create custom datatype to send array of MPI_INTs **/
	MPI_Datatype CUSTOM_ARRAY;
	MPI_Type_contiguous(3, MPI_INT, &CUSTOM_ARRAY);
	MPI_Type_commit(&CUSTOM_ARRAY);

	int dummy_int[3];

	Server *srv=NULL;
	Client *clt=NULL;
	Coordinator *crd=NULL;

	if(rank == 0){
		// Coordinator
		crd = new Coordinator( ReadInputFile("inputfile.txt") );
		int NUM_SERVERS=0;
		while(crd->getnext().at(0).compare("SERVER")==0){
			int server_rank = stoi( crd->get().at(1) ); 
			//cout<<"SERVER "<<server_rank<<" "<<stoi( crd->get().at(2) )<<" "<< stoi( crd->get().at(3))<<"\n";
			init_msg(dummy_int , SERVER_EVENT ,  stoi( crd->get().at(2) ) , stoi( crd->get().at(3) )); //SERVER msg type , left_neighbor_rank . right_neighbor_rank
			MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, server_rank , TAG, MPI_COMM_WORLD);

			++NUM_SERVERS;
		}
		//cout<<"NUM_SERVERS:"<<NUM_SERVERS<<"\n";		
		for(int j=0;j<NUM_SERVERS;++j){
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
			if(dummy_int[0]==ACK){
							crd->servers.push_back(dummy_int[1]);
							//cout<<"ACK received from "<<dummy_int[1]<<"\n";
			}else{
				cout<<"ERROR 0: ACK was expected\n";
			}
		}
		//cout<<"HEREEEEEEEEEEEEEEEEE "<<world_size<<"\n";
		for(int j=0;j<world_size;++j){
			init_msg(dummy_int,ACK,crd->servers.size(),0);
			MPI_Send(&dummy_int, 1, CUSTOM_ARRAY , j , TAG, MPI_COMM_WORLD);
		}
	}else{
		MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, 0, TAG, MPI_COMM_WORLD, &status);
		//cout<<"[rank: "<<rank<<" receive from "<<status.MPI_SOURCE<<"\n";
		if( dummy_int[0] == SERVER_EVENT ){
			srv = new Server(rank,dummy_int[1],dummy_int[2],rank); //set left and right neighbor
			
			init_msg(dummy_int,ACK,rank,0);
			MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, 0 , TAG, MPI_COMM_WORLD);
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, 0, TAG, MPI_COMM_WORLD, &status);
			if(dummy_int[0]!=ACK){
				cout<<"ERROR 2: ACK was expected\n";
			}else{
				srv->serversnum=dummy_int[1];
			}
		}else if( dummy_int[0] == ACK ){
				//cout<<"ACK received\n";
		}else{
			cout<<"ERROR 1: SERVER or ACK was expected\n";
		}
	}
	/*MPI_Barrier(MPI_COMM_WORLD);
	if(rank==0){
		cout<<"worldsize: "<<world_size<<" servers num: "<<crd->servers.size()<<" servers: ";
		for(int i=0;i<crd->servers.size();++i){
			cout<<" ";
			cout<<(crd->servers.at(i) );
			cout<<" ";
		}
		cout<<"\n";
	}*/
	MPI_Barrier(MPI_COMM_WORLD);
	if(rank==0){
		if(crd->get().at(0).compare("START_LEADER_ELECTION")==0){
			//cout<<"START_LEADER_ELECTION servers="<<servers_rank.size()<<"\n";
			for(int j=0;j<crd->servers.size();++j){
				init_msg(dummy_int , START_LEADER_ELECTION , 0 , 0 ); 
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, crd->servers.at(j) , TAG, MPI_COMM_WORLD);
			}
			//cout<<"waiting for leader id\n";
			//MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
			//cout<<dummy_int[0]<<"\n";
		}else{
			cout<<"input file has wrong format"<<" START_LEADER_ELECTION was expected but "<<crd->get().at(0)<<" was occured"<<"\n";
		}
	}else if(srv!=NULL){
		bool issent=false;
		vector<int> types;
		vector<int> ids;
		MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, 0, TAG, MPI_COMM_WORLD, &status);
		if(dummy_int[0]==START_LEADER_ELECTION ){
			//cout<<"LEADER ELECTION MSG received to "<<rank<<"\n";
			issent=true;
			init_msg(dummy_int,CANDIDATE_ID,0,rank);
			MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);	
		}else{
			cout<<"ERROR: START_LEADER_ELECTION was expected\n";
		}
		for(int j=0;j<srv->serversnum;++j){
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, srv->r_neighbor, TAG, MPI_COMM_WORLD, &status);
			if(dummy_int[0]==CANDIDATE_ID){
				srv->servers_list.push_back(dummy_int[2]);
				if(dummy_int[2]>srv->leader_id){
					srv->leader_id=dummy_int[2];
				}
				if(rank!=dummy_int[2]){
					init_msg(dummy_int,CANDIDATE_ID,0,dummy_int[2]);
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
				}
				
			}else{
				cout<<"ERROR: CANDIDATE_ID was expected\n";
			}
		}
		

		/*for(int j=0;j<31;++j){
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
			//if(rank==7){cout<<"RECEIVED from 7 "<<dummy_int[0]<<" "<<dummy_int[2]<<"\n";}
			types.push_back(dummy_int[0]);
			ids.push_back(dummy_int[2]);
			cout<<"rank: "<<rank<< " counter: "<<j<<" ";
			for(int i=0;i<types.size();++i){
				cout<<" ("<<types.at(i)<<","<<ids.at(i)<<") ";
			}
			cout<<"\n\n";


			if(dummy_int[0]==START_LEADER_ELECTION ){
				//cout<<"LEADER ELECTION MSG received to "<<rank<<"\n";
				if(issent==false){
					issent=true;
					init_msg(dummy_int,CANDIDATE_ID,0,rank);
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);	
				}else{
					//counter--;
					//cout<<"\nSPECIAL CASE "<<rank <<" "<<dummy_int[2]<<"\n";
				}
				
			}else if(dummy_int[0]==CANDIDATE_ID){
				srv->servers_list.push_front(dummy_int[2]);
				//cout<<"CANDIDATE FROM "<<status.MPI_SOURCE<<" TO "<<rank<<" WITH LEADER ID "<<dummy_int[2]<<"\n";
				if(issent==false){
					issent=true;
					init_msg(dummy_int,CANDIDATE_ID,0,rank);
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
				}
				if(dummy_int[2]>srv->leader_id){
					srv->leader_id=dummy_int[2];
				}
				if(dummy_int[2]!=rank){
					init_msg(dummy_int,CANDIDATE_ID,0,dummy_int[2]);
					//if(rank==8){cout<<"LEADER "<<2<<"\n";}
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);

				}else{
					//init_msg(dummy_int,START_LEADER_ELECTION,0,srv->leader_id);
					//MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
				}
			}else{cout<<"ERROR:LEADER ELECTION OR CANDIDATE MSG WAS EXPECTED. instead "<<dummy_int[0]<<" was received\n";}
			
			
		}*/

	}

	/*MPI_Barrier(MPI_COMM_WORLD);
	if(rank==5){
		if(srv!=NULL){
			cout<<"\n\n"<<srv->leader_id<<" was elected\t";
			for (std::list<int>::iterator it = srv->servers_list.begin(); it != srv->servers_list.end(); it++){
				cout << *it << ' ';
			}
		}
	}*/

	
	MPI_Barrier(MPI_COMM_WORLD);
	if(srv!=NULL){
		//cout<<"MYLEADER="<<rank<<" "<<srv->leader_id<<endl;
		if(rank==srv->leader_id){
			srv->ld = new Leader();

			int K = (srv->serversnum-3)/4;
			srand( (unsigned)time(NULL) );
			while(K!=0){
				//cout<<"K="<<K<<"\n";
				int r_index = rand()%srv->serversnum;
				//cout <<"K r_index:"<<r_index<<" "<<srv->servers_list.size()<<"\n";
				int tmp = *next(srv->servers_list.begin(),r_index);
				//cout<<"K "<<tmp<<"\n";
				//cout<<"ld->connections size="<<srv->ld->connections.size()<<"tmp="<<tmp<< "r_index="<<r_index<<"\n";
				if(tmp!=rank && tmp!=srv->l_neighbor && tmp!=srv->r_neighbor && contains_rank_v(srv->ld->connections,tmp)==-1){
					K--;

					init_msg(dummy_int,CONNECT,0,0);
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, tmp , TAG, MPI_COMM_WORLD);
					MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, tmp, TAG, MPI_COMM_WORLD, &status);
					if(dummy_int[0]!=ACK){
						cout<<"ERROR:ACK was expected\n";
					}
					srv->ld->connections.push_back(dummy_int[1]);
					cout<<dummy_int[1]<<" CONNECT TO "<<srv->leader_id<<"\n";
				}
			}

			for(int i=1;i<world_size;++i){
				if(contains_rank_v(srv->ld->connections,i)==-1&&i!=srv->leader_id){
					init_msg(dummy_int,ACK,srv->leader_id,0);
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, i , TAG, MPI_COMM_WORLD);
				}
			}
			

		}else{
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, srv->leader_id, TAG, MPI_COMM_WORLD, &status);
			if(dummy_int[0]==CONNECT){
				srv->isconnected=true;
				init_msg(dummy_int,ACK,rank,0);
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->leader_id , TAG, MPI_COMM_WORLD);
			}else if(dummy_int[0]==ACK){
				//do nothing 
				//cout<<"ERROR: UNEXPECTED ACK MSG "<<dummy_int[0]<<"\n"; 
			}else{
				cout<<"ERROR: UNEXPECTED MSG "<<dummy_int[0]<<"\n";
			}
		}
	}else if(rank==0){
		//do nothing
	}else{
		//client case
		MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE, TAG, MPI_COMM_WORLD, &status);
		clt=new Client(dummy_int[1]);
	}


	MPI_Barrier(MPI_COMM_WORLD);
	if(rank==0){	//actions of coordinator 
		for(int j=crd->getindex()+1;j<crd->input.size()+1;++j){
			if(crd->getnext().at(0).compare("UPLOAD")==0){
				int c_dest = stoi( crd->get().at(1) );
				int fd = stoi( crd->get().at(2) );
				//cout<<"CUPLOAD "<<c_dest<<" "<<fd<<"\n";
				init_msg(dummy_int , UPLOAD , fd , 0 ); 
				//cout<<"here"<<j<<endl;
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, c_dest , TAG, MPI_COMM_WORLD);
			}else if(crd->get().at(0).compare("RETRIEVE")==0){
				cout<<crd->get().at(0)<<" "<<crd->get().at(1)<<" "<<crd->get().at(2)<<"\n";
				int c_dest = stoi( crd->get().at(1) );
				int fd = stoi( crd->get().at(2) );
				init_msg(dummy_int , RETRIEVE , fd , 0 ); 
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, c_dest , TAG, MPI_COMM_WORLD);
			}else{
				//cout<<"heree"<<crd->get().at(0)<<" "<<crd->get().at(1)<<" "<<crd->get().at(2)<<"<--\n";
			}
			//cout<<"heree "<<j<<" "<<crd->input.size()+1<<"\n";
			//cout<<"heree"<<crd->get().at(0)<<" "<<crd->get().at(1)<<" "<<crd->get().at(2)<<"<--"<<j<<crd->input.size()+1<<"\n";
		}
		for(int i=1;i<world_size;++i){
			if(contains_rank_v(crd->servers,i)==-1 ){
				crd->clients.push_back(i);
				init_msg(dummy_int , REQUEST_SHUTDOWN , 0 , 0 ); 
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, i , TAG, MPI_COMM_WORLD);
			}
		}
		for(int i=0;i<crd->clients.size();++i){
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, crd->clients.at(i) , TAG, MPI_COMM_WORLD, &status);
			crd->leader=dummy_int[1];
		}
		
		init_msg(dummy_int , REQUEST_SHUTDOWN , 0 , 0 ); 
		//cout<<"CORDINATOR LEADER:"<<crd->leader<<endl;
		MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, crd->leader , TAG, MPI_COMM_WORLD);

	}else if(srv!=NULL){
		if(srv->leader_id==rank){
			while(srv->shutdown==false){	//actions of leader 
				MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE , TAG, MPI_COMM_WORLD, &status);
				//cout<<"UPLOADED3 "<<dummy_int[1]<<"\n";
				if(dummy_int[0]==UPLOAD){			
					//srv->ld->htable[dummy_int[1]]=new h_entry(dummy_int[1],1);
					if( srv->ld->htable.find(dummy_int[1]) == srv->ld->htable.end()){
						int counter =((srv->serversnum-1)/2)+1;
						srv->ld->htable[dummy_int[1]]=new h_entry(dummy_int[1],1);
						srv->ld->htable[dummy_int[1]]->requests.push( new req_entry(status.MPI_SOURCE,counter,UPLOAD,1) );
						//cout<<"UPLOADED4 "<<dummy_int[1]<<"\n";

						//srand( (unsigned)time(NULL) );
						vector<int> tmp_receivers;
						for(int i=0;i<counter;++i){
							int r_index = rand()%srv->serversnum;
							int dest = *next(srv->servers_list.begin(),r_index);
							if(dest!=rank && contains_rank_v(tmp_receivers,dest)==-1 ){
								tmp_receivers.push_back(dest);
								//cout<<"SENT "<<dest<<" "<<"next hop: "<< srv->next_hop(dest)<<" file"<<dummy_int[1]<<" ";
								init_msg(dummy_int , UPLOAD , dummy_int[1] , dest ); 
								MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(dest) , TAG, MPI_COMM_WORLD);
							}else{
								--i;
							}
						}
						//cout<<"\n";
					}else{
						//cout<<"UPLOADED4 FAILED"<<dummy_int[1]<<"\n";
						init_msg(dummy_int , UPLOAD_FAILED , dummy_int[1] , 0 ); 
						MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, status.MPI_SOURCE , TAG, MPI_COMM_WORLD);

					}
				}else if(dummy_int[0]==UPLOAD_ACK){
					--srv->ld->htable[dummy_int[1]]->requests.front()->counter;
					//cout<<"RECEIVED UPLOAD ACK, fid:"<<dummy_int[1]<<" counter: "<<srv->ld->htable[dummy_int[1]]->requests.front()->counter<<" cl: "<<srv->ld->htable[dummy_int[1]]->requests.front()->client_id<<"\n";
					if(srv->ld->htable[dummy_int[1]]->requests.front()->counter==0){
						int dest=srv->ld->htable[dummy_int[1]]->requests.front()->client_id;
						srv->ld->htable[dummy_int[1]]->requests.pop();

						init_msg(dummy_int , UPLOAD_OK , dummy_int[1] , 0 ); 
						//cout<<"SENTING TO CLIENT "<<dest<<" \n";
						MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, dest , TAG, MPI_COMM_WORLD);	
						//cout<<"SENTING TO CLEINT FINISHED"<<dest<<" \n";
					}
				}else if(dummy_int[0]==REQUEST_SHUTDOWN){
					srv->shutdown=true;
					init_msg(dummy_int , REQUEST_SHUTDOWN , 0 , 0 ); 
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
					MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, srv->r_neighbor , TAG, MPI_COMM_WORLD, &status);
					if(dummy_int[0]==REQUEST_SHUTDOWN){
						
					}else{
						cout<<"ERROR:SHUTDOWN was expected but "<<dummy_int[0]<<" was received\n";
					}

				}else if(dummy_int[0]==RETRIEVE){
					if(contains_rank_l(srv->servers_list, dummy_int[1])!=-1 ){
						cout<<"ERROR 9: msg was expected from client but was received from server "<<dummy_int[0]<<"\n";
					}
					//cout<<"LEADER RECEIVED RETRIEVE FROM CLIENT "<<status.MPI_SOURCE<<" , ";
	
					/*map<int, h_entry*>::iterator it;
					for ( it = srv->ld->htable.begin(); it != srv->ld->htable.end(); it++ )
					{
						std::cout << it->first // string (key)
								<< ':'
								<< it->second->f->fd   // string's value 
								<< " " ;
					}
					cout<<endl;
					*/
					if(srv->ld->htable.find(dummy_int[1]) == srv->ld->htable.end()){
						cout<<"CLIENT ETRIEVE FAILED : "<<status.MPI_SOURCE<<endl;
						init_msg(dummy_int , RETRIEVE_FAILED , dummy_int[1] , 0 ); 
						MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, status.MPI_SOURCE , TAG, MPI_COMM_WORLD);
					}else{
						srv->ld->htable[dummy_int[1]]->requests.push(new req_entry(status.MPI_SOURCE,srv->serversnum/2+1,RETRIEVE, srv->ld->htable[dummy_int[1]]->f->version));
						cout<<"RETRIEVE SUCC "<<srv->ld->htable[dummy_int[1]]->f->fd<<"\n";
					}
				}else if(dummy_int[0]==RETRIEVE_ACK){
					cout<<"RETRIEVE_ACK was received "<<srv->ld->htable[dummy_int[1]]->f->fd<<" counter="<<srv->ld->htable[dummy_int[1]]->requests.front()->counter<<" \n";
					srv->ld->htable[dummy_int[1]]->requests.front()->counter--;
					if(srv->ld->htable[dummy_int[1]]->requests.front()->version<dummy_int[2]){
						srv->ld->htable[dummy_int[1]]->requests.front()->version=dummy_int[2];
						srv->ld->htable[dummy_int[1]]->f->version=dummy_int[2];
						cout<<"RETRIEVE_ACK was received END1\n";
					}
					if(srv->ld->htable[dummy_int[1]]->requests.front()->counter==0){
						init_msg(dummy_int , RETRIEVE_OK , dummy_int[1] , srv->ld->htable[dummy_int[1]]->f->version ); 
						cout<<"RETRIEVE_ACK was received END2:client:";cout<<srv->ld->htable[dummy_int[1]]->requests.front()->client_id<<" fid:"<<dummy_int[1]<<"\n";
						MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->ld->htable[dummy_int[1]]->requests.front()->client_id , TAG, MPI_COMM_WORLD);
						srv->ld->htable[dummy_int[1]]->requests.pop();
					}
					cout<<"RETRIEVE_ACK was received END\n";
				}else{
					cout<<"ERROR 7: "<<dummy_int[0]<<"was occured\n";
				}

				//map<int, h_entry*>::iterator it;
				//for ( it = srv->ld->htable.begin(); it != srv->ld->htable.end(); it++ )
				for(auto const &it : srv->ld->htable) 
				{
					if(it.second!=NULL){
						if(it.second->requests.front()!=NULL){
							if(it.second->requests.front()->type==RETRIEVE && it.second->requests.front()->isrunning==false){
								it.second->requests.front()->isrunning=true;
								int N=srv->serversnum/2+1;
								vector<int> dests( select_processes(N,srv->servers_list,rank) );
								cout<<"RETRIEVE"<<it.second->f->fd <<" from "<<it.second->requests.front()->client_id<<":";
								for(int i=0;i<dests.size();++i){
									cout<<dests.at(i)<<" ";
									init_msg(dummy_int , RETRIEVE , it.first , dests.at(i) ); 
									MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(dests.at(i)) , TAG, MPI_COMM_WORLD);	
								}
								cout<<"\n";

							}
						}
					}
				}
				cout<<endl;
			}

		}else{
			while(srv->shutdown==false){	//actions of server 
				MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE , TAG, MPI_COMM_WORLD, &status);
				if(dummy_int[0]==UPLOAD){
					if(rank==dummy_int[2]){
						srv->files.push_back(new file(dummy_int[1],1));
						//cout<<"RECEIVED "<<dummy_int[1]<<" to "<<rank<<"\n";
						init_msg(dummy_int , UPLOAD_ACK , dummy_int[1] , 0 );
						if(srv->isconnected){
							MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->leader_id , TAG, MPI_COMM_WORLD);
						}else{
							MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
						}					
					}else{
						init_msg(dummy_int , UPLOAD , dummy_int[1] , dummy_int[2] ); 
						MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(dummy_int[2]) , TAG, MPI_COMM_WORLD);
					}
				}else if(dummy_int[0]==UPLOAD_ACK){
					init_msg(dummy_int , UPLOAD_ACK , dummy_int[1] , 0 ); 
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
				}else if(dummy_int[0]==REQUEST_SHUTDOWN){
					srv->shutdown=true;
					init_msg(dummy_int , REQUEST_SHUTDOWN , 0 , 0 ); 
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->l_neighbor , TAG, MPI_COMM_WORLD);
				}else if(dummy_int[0]==RETRIEVE){
					if(dummy_int[2]==rank){
						bool exists=false;
						int version=-1;
						for(int i=0;i<srv->files.size();++i){
							if(srv->files.at(i)->fd==dummy_int[1]){
								exists=true;
								version=srv->files.at(i)->version;
							}
						}
						if(exists){
							init_msg(dummy_int , RETRIEVE_ACK , dummy_int[1] , version ); 
							MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(dummy_int[2]) , TAG, MPI_COMM_WORLD);
						}else{
							init_msg(dummy_int , RETRIEVE_ACK , dummy_int[1] , 0 ); 
							MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(dummy_int[2]) , TAG, MPI_COMM_WORLD);
						}

					}else{
						MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(dummy_int[2]) , TAG, MPI_COMM_WORLD);
					}
				}else if(dummy_int[0]==RETRIEVE_ACK){
					MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, srv->next_hop(srv->leader_id) , TAG, MPI_COMM_WORLD);
				}else{
					cout<<"ERROR 6: "<<dummy_int[0]<<"was occured\n";
				}
			}
		}
	}else if(clt!=NULL){ //actions of client
		//clt = new client();
		while(!(clt->active_requests==0&&clt->shutdown==true) ){
			MPI_Recv(&dummy_int, 1, CUSTOM_ARRAY, MPI_ANY_SOURCE , TAG, MPI_COMM_WORLD, &status);
			if(dummy_int[0]==UPLOAD){
				file f(dummy_int[1],1); 
				clt->uploadedfiles.push_back( f );
				clt->active_requests++;
				init_msg(dummy_int,UPLOAD,dummy_int[1],0);
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, clt->leader_id , TAG, MPI_COMM_WORLD);
				//cout<<"UPLOADED2 "<<dummy_int[1]<<"\n";
			}else if(dummy_int[0]==UPLOAD_FAILED){
				clt->active_requests--;
				cout<<"CLIENT "<<rank<<" FAILED TO UPLOAD "<<dummy_int[1]<<"\n";
			}else if(dummy_int[0]==UPLOAD_OK){
				clt->active_requests--;
				cout<<"CLIENT "<<rank<<" UPLOADED "<<dummy_int[1]<<"\n";  
			}else if(dummy_int[0]==RETRIEVE){
				clt->active_requests++;
				MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, clt->leader_id , TAG, MPI_COMM_WORLD);
			}else if(dummy_int[0]==RETRIEVE_OK || dummy_int[0]==RETRIEVE_FAILED){
				clt->active_requests--;
				if(dummy_int[0]==RETRIEVE_OK){
					bool exists=false;
					for(int i=0;i<clt->uploadedfiles.size();++i){
						if(clt->uploadedfiles.at(i).fd==dummy_int[0]){
							exists=true;
							clt->uploadedfiles.at(i).version=dummy_int[2];
						}
					}
					if(exists==false){
						clt->uploadedfiles.push_back(*(new file( dummy_int[1] , dummy_int[2] )) );
					}
					cout<<"CLIENT "<<rank<<" RETRIEVED VERSION "<<dummy_int[2]<<" of "<<dummy_int[1]<<"\n";
				}else if(dummy_int[0]==RETRIEVE_FAILED){
					cout<<"CLIENT "<<rank<<" FAILED TO RETRIEVE "<<dummy_int[1]<<"\n";
				}
			}else if(dummy_int[0]==REQUEST_SHUTDOWN){
				clt->shutdown=true;
			}else{
				cout<<"ERROR 5: "<<dummy_int[0]<<" was occured\n";
			}
		}
		init_msg(dummy_int,REQUEST_SHUTDOWN,clt->leader_id,0);
		MPI_Send(&dummy_int, 1, CUSTOM_ARRAY, 0 , TAG, MPI_COMM_WORLD);
	}
	//cout<<"EXITING "<<rank<<"\n";
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
}



vector<vector<string>>  ReadInputFile(string name){
	vector<vector<string>> input;
	ifstream file(name);
		if(!file){
			cerr<<"ERROR::cannot open inputfile.txt\n";
			exit(1);
		}
		string str; 
		while (getline(file, str))
		{
			vector<string> tmpv;
			istringstream ss(str);
			do { 
				// Read a word 
				string word; 
				ss >> word; 
		
				// Print the read word 
				tmpv.push_back(word);
				// While there is more to read 
			} while (ss);
			
			input.push_back(tmpv);
			
		}
		return input;
} 

void init_msg(int dummy_int[3] , int elem0 , int elem1 , int elem2){
	dummy_int[0]=elem0;
	dummy_int[1]=elem1;
	dummy_int[2]=elem2;
}

int contains_rank_v( vector<int> vect , int rank){
	for(int i=0; i<vect.size() ; ++i){
		if(rank==vect.at(i)){
			return i;
		}
	}
	return -1;
}
int contains_rank_l(list<int> l , int rank){

	int index=0;
	for(std::list<int>::iterator it = l.begin() ; it!=l.end() ; ++it){
		if(*it==rank){
			return index;
		}
		index++;
	}
	return -1;
}

vector<int> select_processes(int N , list<int> l,int rank ){
		vector<int> tmp_receivers;
		for(int i=0;i<N;++i){
			int r_index = rand()%l.size();
			int dest = *next(l.begin(),r_index);
			if(dest!=rank && contains_rank_v(tmp_receivers,dest)==-1 ){
				tmp_receivers.push_back(dest);
			}else{
				--i;
			}
		}
		return tmp_receivers;
}
