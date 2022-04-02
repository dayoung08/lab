#include "header.h"
//initalization : 기존에 있던 SSD와 video 정보 가져오기
//update_new_video: 새로운 비디오 정보 가져오면서, 기존에 있던 video의 인기도, 대역폭도 함께 갱신
//testbed 쪽 함수 고쳐야함...

int bandwidth[SSD_TYPE] = { 3500, 3100, 2400, 1800, 560, 560, 560, 560, 550, 550 };
double DWPD[SSD_TYPE] = { 0.410958904, 0.328767123, 0.290958904, 0.109589041, 0.646575342, 0.328767123, 0.306849315, 0.197260274, 0.109589041, 0.153424658 };

int rand_cnt;

SSD::~SSD() {
	total_assigned_VIDEOs_low_bandwidth_first.clear();
	set<pair<double, int>, less<pair<double, int>>>().swap(total_assigned_VIDEOs_low_bandwidth_first);
}

void placed_video_init_for_simulation(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos, int _num_of_request_per_sec) {
	rand_cnt = 0;
	SSD_initalization_for_simulation(_SSD_list, _num_of_SSDs);
	video_initalization_for_simulation(_VIDEO_CHUNK_list, _num_of_videos, _num_of_request_per_sec);
	//printf("초기화 완료. 이 문구가 빨리 안 뜨면 SSD 숫자를 늘리거나 비디오 세그먼트 수를 줄일 것\n");
}

void SSD_initalization_for_simulation(SSD* _SSD_list, int _num_of_SSDs) {
	std::default_random_engine g(SEED);
	std::uniform_int_distribution<> dist_for_type{ 0, SSD_TYPE-1 }; 
	std::uniform_int_distribution<> dist_for_storage_space{ 0, 4}; // 2 아니면 너무 ADWD 커진다
	dist_for_type.reset();
	dist_for_storage_space.reset();
	//std::uniform_int_distribution<> dist_for_bandwidth{ 550, 3500 };
	//std::uniform_int_distribution<> dist_for_DWPD{ 11, 65 };
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		int ssd_index = ssd;
		_SSD_list[ssd_index].index = ssd_index;
		if (ssd == VIRTUAL_SSD) {
			_SSD_list[VIRTUAL_SSD].storage_capacity = INFINITY;
			_SSD_list[VIRTUAL_SSD].DWPD = INFINITY;
			_SSD_list[VIRTUAL_SSD].maximum_bandwidth = -INFINITY;
		}
		else {
			int r = dist_for_type(g);
			//int r = ssd % SSD_TYPE;
			//https://mapoo.net/os/oswindows/hdd-ssd-%EC%9A%A9%EB%9F%89-%EA%B3%84%EC%82%B0/
			_SSD_list[ssd_index].storage_capacity = (double)250000 * 0.9313 * pow(2, dist_for_storage_space(g));

			_SSD_list[ssd_index].DWPD = DWPD[r];
			_SSD_list[ssd_index].maximum_bandwidth = bandwidth[r];
			//_SSD_list[ssd_index].DWPD = dist_for_DWPD(g);
			//_SSD_list[ssd_index].maximum_bandwidth = dist_for_bandwidth(g)/ 100;
		}
		//https://tekie.com/blog/hardware/ssd-vs-hdd-speed-lifespan-and-reliability/
		//https://www.quora.com/What-is-the-average-read-write-speed-of-an-SSD-hard-drive

		_SSD_list[ssd_index].storage_usage = 0;
		_SSD_list[ssd_index].total_bandwidth_usage = 0;
		//_SSD_list[ssd_index].serviced_bandwidth_usage = 0;
		_SSD_list[ssd_index].ADWD = 0;
		//_SSD_list[ssd_index].daily_write_MB = 0;
		_SSD_list[ssd_index].total_write_MB = 0;
		_SSD_list[ssd_index].running_days = 1; // 첫 날 = 1이니까

		_SSD_list[ssd_index].node_hostname = "datanode" + to_string(ssd);
	}
}

void video_initalization_for_simulation(VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_videos, int _num_of_request_per_sec) {
	double* vid_pop = set_zipf_pop(_num_of_videos, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + _num_of_videos);
	std::default_random_engine g(SEED);
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		_VIDEO_CHUNK_list[video_index].index = video_index;
		_VIDEO_CHUNK_list[video_index].size = (double)VIDEO_SIZE;
		_VIDEO_CHUNK_list[video_index].once_bandwidth = (double)VIDEO_BANDWIDTH;
		_VIDEO_CHUNK_list[video_index].type = 1; // 시뮬레이션에서는 안 쓰므로 통일

		double pop = vid_pop_shuffle.back();
		vid_pop_shuffle.pop_back();
		_VIDEO_CHUNK_list[video_index].popularity = pop;
		_VIDEO_CHUNK_list[video_index].requested_bandwidth = pop * (double)_num_of_request_per_sec * _VIDEO_CHUNK_list[video_index].once_bandwidth; //220124
		_VIDEO_CHUNK_list[video_index].assigned_SSD = NONE_ALLOC;
		//_VIDEO_CHUNK_list[video_index].is_serviced = false;
		_VIDEO_CHUNK_list[video_index].path = "/segment_" + to_string(video_index) + ".mp4";
	}
	delete[] vid_pop;
	vid_pop_shuffle.clear();
	vector<double>().swap(vid_pop_shuffle); //메모리 해제를 위해
}

void migrated_video_init_for_simulation(SSD* _SSD_list, VIDEO_CHUNK* _existed_VIDEO_CHUNK_list, VIDEO_CHUNK* _new_VIDEO_CHUNK_list, int _migration_method, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos, int _num_of_request_per_sec, int _time) {
	double* vid_pop = set_zipf_pop(_num_of_existed_videos + _num_of_new_videos, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + _num_of_existed_videos + _num_of_new_videos);
	default_random_engine g(SEED + rand_cnt);
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.begin() + _num_of_existed_videos, g);
	std::shuffle(vid_pop_shuffle.begin() + _num_of_existed_videos, vid_pop_shuffle.end(), g);

	//우선 전부 제거함. 재계산을 위함.
	for (int ssd = 0; ssd <= _num_of_SSDs; ssd++) {
		int ssd_index = ssd;
		_SSD_list[ssd_index].total_assigned_VIDEOs_low_bandwidth_first.clear();
		_SSD_list[ssd_index].storage_usage = 0;
		_SSD_list[ssd_index].total_bandwidth_usage = 0;
		//_SSD_list[ssd_index].serviced_bandwidth_usage = 0;
		if(_time==1)
			_SSD_list[ssd_index].running_days++;
		_SSD_list[ssd_index].ADWD = (_SSD_list[ssd_index].total_write_MB / (_SSD_list[ssd_index].DWPD * _SSD_list[ssd_index].storage_capacity)) / _SSD_list[ssd_index].running_days;

		if (ssd == VIRTUAL_SSD) {
			_SSD_list[ssd_index].total_write_MB = 0;
		}
	}

	for (int vid = 0; vid < _num_of_existed_videos + _num_of_new_videos; vid++) {
		int video_index = vid;
		double pop = vid_pop_shuffle.back();
		vid_pop_shuffle.pop_back();

		if (video_index < _num_of_existed_videos) { // 기존에 있던 영상의 인기도, 밴드윗 갱신
			_existed_VIDEO_CHUNK_list[video_index].popularity = pop;
			_existed_VIDEO_CHUNK_list[video_index].requested_bandwidth = pop * (double)_num_of_request_per_sec * _existed_VIDEO_CHUNK_list[video_index].once_bandwidth; //220124
			int SSD_index = _existed_VIDEO_CHUNK_list[video_index].assigned_SSD;
			//_existed_VIDEO_CHUNK_list[video_index].is_serviced = false; // 기존에 있었어도 일단 초기화
			
			if (_migration_method >= MIGRATION_OURS) {
				if (SSD_index != NONE_ALLOC) {
					_SSD_list[SSD_index].storage_usage += _existed_VIDEO_CHUNK_list[video_index].size;
					_SSD_list[SSD_index].total_bandwidth_usage += _existed_VIDEO_CHUNK_list[video_index].requested_bandwidth;
					_SSD_list[SSD_index].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_existed_VIDEO_CHUNK_list[video_index].requested_bandwidth, video_index));
				}
				else {
					_existed_VIDEO_CHUNK_list[video_index].assigned_SSD = VIRTUAL_SSD;
					_SSD_list[VIRTUAL_SSD].storage_usage += _existed_VIDEO_CHUNK_list[video_index].size;
					_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _existed_VIDEO_CHUNK_list[video_index].requested_bandwidth;
					_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_existed_VIDEO_CHUNK_list[video_index].requested_bandwidth, video_index));
				}
			}
		}
		else { // 새로운 영상
			_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].index = video_index;
			_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].size = (double) VIDEO_SIZE;
			_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].once_bandwidth = (double) VIDEO_BANDWIDTH;
			_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].popularity = pop;
			_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].requested_bandwidth = pop * (double)_num_of_request_per_sec * _new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].once_bandwidth; //220124
			_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].path = "/segment_" + to_string(video_index) + ".mp4";
			//_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].is_serviced = false; 

			//이 아래는 migration scheme 쓸 때 사용함. vitual ssd에 넣어놓음
			if (_migration_method >= MIGRATION_OURS) {
				_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].assigned_SSD = VIRTUAL_SSD;
				_SSD_list[VIRTUAL_SSD].storage_usage += _new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].size;
				_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].requested_bandwidth;
				_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].requested_bandwidth, video_index));
			}
			else {
				_new_VIDEO_CHUNK_list[video_index - _num_of_existed_videos].assigned_SSD = NONE_ALLOC;
			}
		}
	}

	//cout << _existed_VIDEO_CHUNK_list[1].popularity << endl;
	//cout << _existed_VIDEO_CHUNK_list[10].popularity << endl;
	delete[] vid_pop;
	vid_pop_shuffle.clear();
	vector<double>().swap(vid_pop_shuffle); //메모리 해제를 위해
}

void placed_video_init_for_testbed(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _num_of_SSDs, int _num_of_videos, int _num_of_request_per_sec) {
	SSD_initalization_for_testbed(_SSD_list, _num_of_SSDs);
	video_initalization_for_testbed(_VIDEO_CHUNK_list, _num_of_videos, _num_of_request_per_sec, true);

	double* vid_pop = set_zipf_pop(_num_of_videos, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + _num_of_videos);
	default_random_engine g(SEED + rand_cnt);
	shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		double pop = vid_pop[video_index];
		_VIDEO_CHUNK_list[video_index].popularity = pop;
		_VIDEO_CHUNK_list[video_index].requested_bandwidth = pop * _num_of_request_per_sec * _VIDEO_CHUNK_list[video_index].once_bandwidth;
	}
}

void migrated_video_init_for_testbed(SSD* _SSD_list, VIDEO_CHUNK* _existed_VIDEO_CHUNK_list, VIDEO_CHUNK* _new_VIDEO_CHUNK_list, int _migration_method, int& _num_of_SSDs, int& _num_of_existed_videos, int& _num_of_new_videos, int _num_of_request_per_sec, bool _has_new_files) {
	SSD_initalization_for_testbed(_SSD_list, _num_of_SSDs);
	video_initalization_for_testbed(_existed_VIDEO_CHUNK_list, _num_of_existed_videos, _num_of_request_per_sec, false);
	if (_has_new_files) {
		video_initalization_for_testbed(_new_VIDEO_CHUNK_list, _num_of_new_videos, _num_of_request_per_sec, true);
	}
	else {
		_num_of_new_videos = 0;
	}

	//기존의 엉상, 추가 영상을 합해서 인기도, 밴드윗 업데이트
	//아래는 랜덤 대역폭 변화를 위한 인기도 설정
	double* vid_pop = set_zipf_pop(_num_of_existed_videos + _num_of_new_videos, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + _num_of_existed_videos + _num_of_new_videos);
	default_random_engine g(SEED + rand_cnt);
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.begin() + _num_of_existed_videos, g);
	std::shuffle(vid_pop_shuffle.begin() + _num_of_existed_videos, vid_pop_shuffle.end(), g);
	//대역폭 설정 시작
	for (int vid = 0; vid < _num_of_existed_videos + _num_of_new_videos; vid++) {
		int video_index = vid;
		double pop = vid_pop_shuffle[video_index];
		if (video_index < _num_of_existed_videos) { 
			// 기존에 있던 영상의 밴드윗 갱신
			_existed_VIDEO_CHUNK_list[video_index].popularity = pop;
			_existed_VIDEO_CHUNK_list[video_index].requested_bandwidth = _existed_VIDEO_CHUNK_list[video_index].popularity * _num_of_request_per_sec * _existed_VIDEO_CHUNK_list[video_index].once_bandwidth;

			//SSD 관련 수치 다시 계산
			int SSD_index = _existed_VIDEO_CHUNK_list[video_index].assigned_SSD;
			if (SSD_index != NONE_ALLOC) {
				_SSD_list[SSD_index].storage_usage += _existed_VIDEO_CHUNK_list[video_index].size;
				_SSD_list[SSD_index].total_bandwidth_usage += _existed_VIDEO_CHUNK_list[video_index].requested_bandwidth;
				_SSD_list[SSD_index].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_existed_VIDEO_CHUNK_list[video_index].requested_bandwidth, video_index));
			}
			else {
				_existed_VIDEO_CHUNK_list[video_index].assigned_SSD = VIRTUAL_SSD;
				_SSD_list[VIRTUAL_SSD].storage_usage += _existed_VIDEO_CHUNK_list[video_index].size;
				_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _existed_VIDEO_CHUNK_list[video_index].requested_bandwidth;
				_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_existed_VIDEO_CHUNK_list[video_index].requested_bandwidth, video_index));
			}
		}
		else {	
			//새로운 영상의 밴드윗 갱신
			_new_VIDEO_CHUNK_list[video_index].popularity = pop;
			_new_VIDEO_CHUNK_list[video_index].requested_bandwidth = _new_VIDEO_CHUNK_list[video_index].popularity * _num_of_request_per_sec * _new_VIDEO_CHUNK_list[video_index].once_bandwidth;

			//새로운 파일들은 가상 스토리지에 삽입함
			_new_VIDEO_CHUNK_list[video_index].assigned_SSD = VIRTUAL_SSD;
			_SSD_list[VIRTUAL_SSD].storage_usage += _new_VIDEO_CHUNK_list[video_index].size;
			_SSD_list[VIRTUAL_SSD].total_bandwidth_usage += _new_VIDEO_CHUNK_list[video_index].requested_bandwidth;
			_SSD_list[VIRTUAL_SSD].total_assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_new_VIDEO_CHUNK_list[video_index].requested_bandwidth, video_index));
		}
	}
	vid_pop_shuffle.clear();
	vector<double>().swap(vid_pop_shuffle); //메모리 해제를 위해
	delete[] vid_pop;//여기까지 인기도 설정 완료

}


void SSD_initalization_for_testbed(SSD* _SSD_list, int& _num_of_SSDs) {
	ifstream fin_ssd("SSD_list.in"); // fin 객체 생성(cin 처럼 이용!) -> 셸 프로그래밍 파일 실행시, 이 파일이 없으면 생성하고(가볍게 미리 만든 디폴트 파일 copy하자), storage_usage, bandwidth_usage, write_MB가 0이도록 함
	if (fin_ssd.is_open())
	{
		string str;
		int cnt = 0;
		while (getline(fin_ssd, str)) // 파일이 끝날때까지 한 줄씩 읽어오기
		{
			if (cnt == 0) {
				_num_of_SSDs = stoi(str);
				_SSD_list = new SSD[_num_of_SSDs + 1];
			}
			else {
				int ssd_index = cnt;
				_SSD_list[ssd_index].index = ssd_index;
				string* ssd_info = split(str, '\t');
				_SSD_list[ssd_index].node_hostname = ssd_info[0]; // datanode 이름임. TLC 1개, QLC하나가 같은 datanode hostname을 가짐
				_SSD_list[ssd_index].storage_capacity = stod(ssd_info[1]); //검색해서 미리 입력해놓기
				_SSD_list[ssd_index].maximum_bandwidth = stod(ssd_info[2]); //검색해서 미리 입력해놓기
				_SSD_list[ssd_index].DWPD = stod(ssd_info[3]); //검색해서 미리 입력해놓기
				// WAF = (Attrib_247 + Attrib_248) / Attrib_247로 계산하고,
				//위의 값들은 smartctl -a /dev/sda 으로 읽어오기 가능함. https://community.ui.com/questions/Using-an-SSD-for-CloudKey-Gen2-Protect/b91b418f-ab93-42ba-8d0d-31b568f50bd9
				_SSD_list[ssd_index].total_write_MB = stod(ssd_info[4]); // smartctl -a /dev/sda로 읽어오기 가능함 Total_LBAs_Written
				_SSD_list[ssd_index].running_days = stoi(ssd_info[5]); // smartctl -a /dev/sda로 읽어오기 가능함 Power_On_Hours
				_SSD_list[ssd_index].ADWD = (_SSD_list[ssd_index].total_write_MB / (_SSD_list[ssd_index].storage_capacity * _SSD_list[ssd_index].DWPD)) / _SSD_list[ssd_index].running_days;
			}
			cnt++;
		}
	}
	fin_ssd.close(); // 파일 닫기
	//파일 추가에 사용될 virtual storage
	_SSD_list[VIRTUAL_SSD].storage_capacity = INFINITY;
	_SSD_list[VIRTUAL_SSD].DWPD = INFINITY;
	_SSD_list[VIRTUAL_SSD].maximum_bandwidth = -INFINITY;
	_SSD_list[VIRTUAL_SSD].storage_usage = 0;
	_SSD_list[VIRTUAL_SSD].total_bandwidth_usage = 0;
	//_SSD_list[VIRTUAL_SSD].serviced_bandwidth_usage = 0;
	_SSD_list[VIRTUAL_SSD].ADWD = 0;
	_SSD_list[VIRTUAL_SSD].total_write_MB = 0;
	_SSD_list[VIRTUAL_SSD].running_days = 1; // 첫 날 = 1이니까
	_SSD_list[VIRTUAL_SSD].node_hostname = "virtual_node";
}

void video_initalization_for_testbed(VIDEO_CHUNK* _VIDEO_CHUNK_list, int& num_of_videos, int _num_of_request_per_sec, bool is_new) {
	ifstream fin_video("uploaded_video_list.in"); // fin 객체 생성(cin 처럼 이용!)
	if (fin_video.is_open()) {
		string str;
		int cnt = -1;
		while (getline(fin_video, str)) // 파일이 끝날때까지 한 줄씩 읽어오기
		{
			if (cnt == -1) {
				num_of_videos = stoi(str);
				_VIDEO_CHUNK_list = new VIDEO_CHUNK[num_of_videos];
			}
			else {
				int video_index = cnt;
				_VIDEO_CHUNK_list[video_index].index = video_index;
				string* video_info = split(str, '\t');

				_VIDEO_CHUNK_list[video_index].type = stoi(video_info[0]);
				_VIDEO_CHUNK_list[video_index].path = video_info[1];
				_VIDEO_CHUNK_list[video_index].size = stod(video_info[2]);
				_VIDEO_CHUNK_list[video_index].once_bandwidth = stod(video_info[3]);
				if (is_new)
					_VIDEO_CHUNK_list[video_index].assigned_SSD = NONE_ALLOC;
				else
					_VIDEO_CHUNK_list[video_index].assigned_SSD = stoi(video_info[4]);
			}
			cnt++;
		}
	}
	fin_video.close(); // 파일 닫기
}


double* set_zipf_pop(int length, double alpha, double beta) {
	double* zipf = new double[length];
	double* pop = new double[length];
	double sum_caculatedValue = 0;
	double caculatedValue = 0;

	zipf[0] = 0;
	for (int i = 1; i < length + 1; i++) {
		caculatedValue = (double)beta / powl(i, alpha);
		sum_caculatedValue += caculatedValue;
		zipf[i - 1] = caculatedValue;
	}
	double sum = 0;
	for (int i = 1; i < length + 1; i++) {
		zipf[i - 1] /= sum_caculatedValue;
		pop[length - i] = zipf[i - 1];
	}
	delete[] zipf;
	return pop;
}

bool is_full_storage_space(SSD* _SSD_list, VIDEO_CHUNK* _VIDEO_CHUNK_list, int _to_ssd, int _from_vid) {
	return (_SSD_list[_to_ssd].storage_usage + _VIDEO_CHUNK_list[_from_vid].size) > _SSD_list[_to_ssd].storage_capacity;
}

//c++은 split 없어서 인터넷에서 복붙했다 ㅋㅋㅋㅋ....
string* split(string str, char Delimiter) {
	istringstream iss(str);             // istringstream에 str을 담는다.
	string buffer;                      // 구분자를 기준으로 절삭된 문자열이 담겨지는 버퍼
	vector<string> result;

	// istringstream은 istream을 상속받으므로 getline을 사용할 수 있다.
	while (getline(iss, buffer, Delimiter)) {
		result.push_back(buffer);               // 절삭된 문자열을 vector에 저장
	}
	
	string* result_array = new string[result.size()];
	copy(result.begin(), result.end(), result_array);
	result.clear();
	vector<string>().swap(result); //메모리 해제를 위해
	return result_array;
}

void growing_cnt() {
	rand_cnt++;
}