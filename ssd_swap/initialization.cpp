#include "header.h"
//initalization : ������ �ִ� SSD�� video ���� ��������
//update_new_video: ���ο� ���� ���� �������鼭, ������ �ִ� video�� �α⵵, �뿪���� �Բ� ����

#define MAX_DWPD 150 //1.50  // for simulation
#define MIN_DWPD 4   //0.04  // for simulation

#define MAX_WAF 40 // 4.0  // for simulation
#define MIN_WAF 10 // 1.0   // for simulation //https://www.crucial.com/support/articles-faq-ssd/why-does-SSD-seem-to-be-wearing-prematurely
//https://www.samsung.com/semiconductor/global.semi.static/Multi-stream_Cassandra_Whitepaper_Final-0.pdf �̰� 1~3.2. 1�̸� �׳� ������ �ݷ������� ���� ���� ������ ���°�

#define MAX_SSD_BANDWIDTH 5000 // for simulation
#define MIN_SSD_BANDWIDTH 400 // for simulation

double video_bandwidth_once_usage = 1.25; //������ 10000kbps(�� 10Mbps)��� �����غ��ô�.10*0.125=1.25,�ϳ��� 1.25MB/s�� ��ϴ�.
double size_of_video = 6; //���׸�Ʈ�� 6��¥����� �����ϸ�, 1MB/s x 6s = 6MB

int rand_cnt = 0;
void initalization_for_simulation(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_videos) {
	for (int ssd = 0; ssd < _num_of_SSDs; ssd++) {
		int ssd_index = ssd;
		_SSD_list[ssd_index].index = ssd_index;
		_SSD_list[ssd_index].storage_capacity = 500000 * pow(2, rand() % 4); // 0.5, 1, 2, 4TB
		//_SSD_list[index].storage_space = 2000000 * 0.9095;  //���� 2�׶�� �ణ �� ��������

		_SSD_list[ssd_index].DWPD = ((double)(rand() % (MAX_DWPD - MIN_DWPD + 1) + MIN_DWPD)) / 100;
		_SSD_list[ssd_index].WAF = ((double)(rand() % (MAX_WAF - MIN_WAF + 1) + MIN_WAF)) / 10;
		_SSD_list[ssd_index].DWPD /= _SSD_list[ssd_index].WAF;
		_SSD_list[ssd_index].maximum_bandwidth = rand() % (MAX_SSD_BANDWIDTH - MIN_SSD_BANDWIDTH + 1) + MIN_SSD_BANDWIDTH;

		//https://tekie.com/blog/hardware/ssd-vs-hdd-speed-lifespan-and-reliability/
		//https://www.quora.com/What-is-the-average-read-write-speed-of-an-SSD-hard-drive

		_SSD_list[ssd_index].storage_usage = 0;
		_SSD_list[ssd_index].bandwidth_usage = 0;
		_SSD_list[ssd_index].ADWD = 0;
		_SSD_list[ssd_index].daily_write_MB = 0;

		_SSD_list[ssd_index].total_write_MB = 0;
		_SSD_list[ssd_index].running_days = 1; // ù �� = 1�̴ϱ�

		_SSD_list[ssd_index].node_hostname = "datanode" + to_string(ssd+1);
	}

	/*double cal = (VIDEO_BANDWIDTH_USAGE * _num_of_videos);
	if (cal > total_maximum_bandwidth) {
		printf("���׸�Ʈ ������ ����� �� ���� SSD ����� �� �պ��� ŭ\n");
		exit(0);
	}*/

	double* vid_pop = set_zipf_pop(_num_of_videos, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + _num_of_videos);
	std::mt19937 g(SEED + rand_cnt);
	rand_cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
	for (int vid = 0; vid < _num_of_videos; vid++) {
		int video_index = vid;
		_VIDEO_SEGMENT_list[video_index].index = video_index;
		_VIDEO_SEGMENT_list[video_index].size = size_of_video;
		_VIDEO_SEGMENT_list[video_index].once_bandwidth = video_bandwidth_once_usage;

		//double pop = vid_pop[video_index];
		double pop = vid_pop_shuffle.back();
		vid_pop_shuffle.pop_back();
		_VIDEO_SEGMENT_list[video_index].popularity = pop;
		_VIDEO_SEGMENT_list[video_index].requested_bandwidth = pop * (double) NUM_OF_REQUEST_PER_SEC * video_bandwidth_once_usage; //220124

		_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
		_VIDEO_SEGMENT_list[video_index].is_alloc = false;

		_VIDEO_SEGMENT_list[video_index].path = "/segment_" + to_string(video_index) + ".mp4";
		//������� �Ҵ�
	}
	delete[] vid_pop;
	vid_pop_shuffle.clear();
	vector<double>().swap(vid_pop_shuffle); //�޸� ������ ����
	printf("�ʱ�ȭ �Ϸ�. �� ������ ���� �� �߸� SSD ���ڸ� �ø��ų� ���� ���׸�Ʈ ���� ���� ��\n");
}

void update_new_video_for_simulation(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos, int _num_of_new_videos) {
	double* vid_pop = set_zipf_pop( _num_of_existed_videos + _num_of_new_videos, ALPHA, BETA);
	vector<double>vid_pop_shuffle(vid_pop, vid_pop + _num_of_existed_videos + _num_of_new_videos);
	std::mt19937 g(SEED + rand_cnt);
	rand_cnt++;
	std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);

	for (int ssd = 0; ssd < _num_of_SSDs; ssd++) {
		int ssd_index = ssd;
		_SSD_list[ssd_index].bandwidth_usage = 0;
		_SSD_list[ssd_index].assigned_VIDEOs_low_bandwidth_first.clear();
	}

	for (int vid = 0; vid < _num_of_existed_videos + _num_of_new_videos; vid++) {
		int video_index = vid;
		//double pop = vid_pop[video_index];
		double pop = vid_pop_shuffle.back();
		vid_pop_shuffle.pop_back();

		if (video_index < _num_of_existed_videos) { // ������ �ִ� ������ �α⵵, ����� ����
			_existed_VIDEO_SEGMENT_list[video_index].popularity = pop;
			_existed_VIDEO_SEGMENT_list[video_index].requested_bandwidth = pop * (double) NUM_OF_REQUEST_PER_SEC * video_bandwidth_once_usage; //220124
			int SSD_index = _existed_VIDEO_SEGMENT_list[video_index].assigned_SSD;
			if (SSD_index != NONE_ALLOC) {
				_SSD_list[SSD_index].bandwidth_usage += _existed_VIDEO_SEGMENT_list[video_index].requested_bandwidth;
				_SSD_list[SSD_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_existed_VIDEO_SEGMENT_list[video_index].requested_bandwidth, video_index));
			}
		}
		else { // ���ο� ����
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].index = video_index;
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].size = size_of_video;
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].once_bandwidth = video_bandwidth_once_usage;
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].popularity = pop;
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].requested_bandwidth = pop * NUM_OF_REQUEST_PER_SEC * video_bandwidth_once_usage; //220124
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].assigned_SSD = NONE_ALLOC;
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].is_alloc = false;
			_new_VIDEO_SEGMENT_list[video_index - _num_of_existed_videos].path = "/segment_" + to_string(video_index) + ".mp4";
		}
	}
	delete[] vid_pop;
	vid_pop_shuffle.clear();
	vector<double>().swap(vid_pop_shuffle); //�޸� ������ ����
}

void initalization_for_testbed(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int& num_of_SSDs, int& num_of_videos) {
	//���� �о���� ���·� �� ������.
	//https://tang2.tistory.com/335
	ifstream fin_ssd("SSD_list.in"); // fin ��ü ����(cin ó�� �̿�!) -> �� ���α׷��� ���� �����, �� ������ ������ �����ϰ�(������ �̸� ���� ����Ʈ ���� copy����), storage_usage, bandwidth_usage, write_MB�� 0�̵��� ��
	if (fin_ssd.is_open())
	{
		string str;
		int cnt = -1;
		while (getline(fin_ssd, str)) // ������ ���������� �� �پ� �о����
		{
			if (cnt == -1) {
				num_of_SSDs = stoi(str);
				_SSD_list = new SSD[num_of_SSDs];
			}
			else {
				int ssd_index = cnt;
				_SSD_list[ssd_index].index = ssd_index;

				string* ssd_info = split(str, '\t');
				_SSD_list[ssd_index].node_hostname = ssd_info[0];
				_SSD_list[ssd_index].storage_capacity = stod(ssd_info[1]);
				_SSD_list[ssd_index].maximum_bandwidth = stod(ssd_info[2]);
				_SSD_list[ssd_index].DWPD = stod(ssd_info[3]);
				_SSD_list[ssd_index].WAF = stod(ssd_info[4]);
				// WAF = (Attrib_247 + Attrib_248) / Attrib_247�� ����ϰ�,
				//���� ������ smartctl -a /dev/sda ���� �о���� ������. https://community.ui.com/questions/Using-an-SSD-for-CloudKey-Gen2-Protect/b91b418f-ab93-42ba-8d0d-31b568f50bd9
				_SSD_list[ssd_index].DWPD /= _SSD_list[ssd_index].WAF;

				_SSD_list[ssd_index].storage_usage = stod(ssd_info[5]);
				_SSD_list[ssd_index].bandwidth_usage = stod(ssd_info[6]);
				_SSD_list[ssd_index].daily_write_MB = stod(ssd_info[7]);
				_SSD_list[ssd_index].ADWD = _SSD_list[ssd_index].daily_write_MB / (_SSD_list[ssd_index].storage_capacity * _SSD_list[ssd_index].DWPD);
				_SSD_list[ssd_index].total_write_MB = stod(ssd_info[8]);
				_SSD_list[ssd_index].running_days = stod(ssd_info[9]);
			}
			cnt++;
		}
	}
	fin_ssd.close(); // ���� �ݱ�

	ifstream fin_video("existed_video_list.in"); // fin ��ü ����(cin ó�� �̿�!)
	if (fin_video.is_open())
	{
		double* vid_pop = NULL;
		vector<double>vid_pop_shuffle;
		string str;
		int cnt = -1;
		while (getline(fin_video, str)) // ������ ���������� �� �پ� �о����
		{
			if (cnt == -1) {
				num_of_videos = stoi(str);
				_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[num_of_videos];
				vid_pop = set_zipf_pop(num_of_videos, ALPHA, BETA);
				vid_pop_shuffle = vector<double>(vid_pop, vid_pop + num_of_videos);
				std::mt19937 g(SEED + rand_cnt);
				rand_cnt++;
				std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
			}
			else {
				int video_index = cnt;
				_VIDEO_SEGMENT_list[video_index].index = video_index;

				string* video_info = split(str, '\t');
				_VIDEO_SEGMENT_list[video_index].path = video_info[0];
				_VIDEO_SEGMENT_list[video_index].size = stod(video_info[1]);
				_VIDEO_SEGMENT_list[video_index].once_bandwidth = stod(video_info[2]);
				double pop = vid_pop_shuffle.back();
				vid_pop_shuffle.pop_back();
				//double pop = vid_pop[video_index];
				_VIDEO_SEGMENT_list[video_index].popularity = pop;
				_VIDEO_SEGMENT_list[video_index].requested_bandwidth = pop * NUM_OF_REQUEST_PER_SEC * _VIDEO_SEGMENT_list[video_index].once_bandwidth;
				if (!strcmp(video_info[3].c_str(), "NONE_ALLOC")) {
					_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
					_VIDEO_SEGMENT_list[video_index].is_alloc = false;
				}
				else {
					_VIDEO_SEGMENT_list[video_index].assigned_SSD = stod(video_info[3]);
					_VIDEO_SEGMENT_list[video_index].is_alloc = true;
				}
				//������� �Ҵ�
			}
			cnt++;
		}
		delete[] vid_pop;
		vid_pop_shuffle.clear();
		vector<double>().swap(vid_pop_shuffle); //�޸� ������ ����
	}
	fin_video.close(); // ���� �ݱ�
}

void update_new_video_for_testbed(SSD* _SSD_list, VIDEO_SEGMENT* _existed_VIDEO_SEGMENT_list, VIDEO_SEGMENT* _new_VIDEO_SEGMENT_list, int _num_of_SSDs, int _num_of_existed_videos, int& _num_of_new_videos) {
	double* vid_pop = NULL;
	vector<double>vid_pop_shuffle;

	ifstream fin_video("new_video_list.in"); // fin ��ü ����(cin ó�� �̿�)
	if (fin_video.is_open())
	{
		string str;
		int cnt = -1;
		while (getline(fin_video, str)) // ������ ���������� �� �پ� �о����
		{
			if (cnt == -1) {
				_num_of_new_videos = stoi(str);
				_new_VIDEO_SEGMENT_list = new VIDEO_SEGMENT[_num_of_new_videos];
				vid_pop = set_zipf_pop(_num_of_existed_videos + _num_of_new_videos, ALPHA, BETA);
				vid_pop_shuffle = vector<double>(vid_pop, vid_pop + _num_of_existed_videos + _num_of_new_videos);
				std::mt19937 g(SEED + rand_cnt);
				rand_cnt++;
				std::shuffle(vid_pop_shuffle.begin(), vid_pop_shuffle.end(), g);
			}
			else {
				int video_index = _num_of_existed_videos + cnt;
				_new_VIDEO_SEGMENT_list[video_index].index = video_index;

				string* video_info = split(str, '\t');
				_new_VIDEO_SEGMENT_list[video_index].path = video_info[0];
				_new_VIDEO_SEGMENT_list[video_index].size = stod(video_info[1]);
				_new_VIDEO_SEGMENT_list[video_index].once_bandwidth = stod(video_info[2]);
				double pop = vid_pop_shuffle.back();
				vid_pop_shuffle.pop_back();
				//double pop = vid_pop[video_index];
				_new_VIDEO_SEGMENT_list[video_index].popularity = pop;
				_new_VIDEO_SEGMENT_list[video_index].requested_bandwidth = pop * NUM_OF_REQUEST_PER_SEC * _new_VIDEO_SEGMENT_list[video_index].once_bandwidth;
				_new_VIDEO_SEGMENT_list[video_index].assigned_SSD = NONE_ALLOC;
				_new_VIDEO_SEGMENT_list[video_index].is_alloc = false;
				//������� �Ҵ�
			}
			cnt++;
		}
		delete[] vid_pop;
		vid_pop_shuffle.clear();
		vector<double>().swap(vid_pop_shuffle); //�޸� ������ ����
	}
	fin_video.close(); // ���� �ݱ�

	//������ ���� ������ �α⵵, ����� ����
	for (int ssd = 0; ssd < _num_of_SSDs; ssd++) {
		int ssd_index = ssd;
		_SSD_list[ssd_index].bandwidth_usage = 0;
		_SSD_list[ssd_index].assigned_VIDEOs_low_bandwidth_first.clear();
	}

	for (int vid = 0; vid < _num_of_existed_videos; vid++) {
		int video_index = vid;
		double pop = vid_pop[video_index];
		/*double pop = vid_pop_shuffle.back();
		vid_pop_shuffle.pop_back();*/

		_existed_VIDEO_SEGMENT_list[video_index].popularity = pop;
		_existed_VIDEO_SEGMENT_list[video_index].requested_bandwidth = pop * NUM_OF_REQUEST_PER_SEC * video_bandwidth_once_usage; //220124
		int SSD_index = _existed_VIDEO_SEGMENT_list[video_index].assigned_SSD;
		if (SSD_index != NONE_ALLOC) {
			_SSD_list[SSD_index].bandwidth_usage += _existed_VIDEO_SEGMENT_list[video_index].requested_bandwidth;
			_SSD_list[SSD_index].assigned_VIDEOs_low_bandwidth_first.insert(make_pair(_existed_VIDEO_SEGMENT_list[video_index].requested_bandwidth, video_index));
		}
	}
	delete[] vid_pop;
	//vid_pop_shuffle.clear();
	//vector<double>().swap(vid_pop_shuffle); //�޸� ������ ����
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

bool is_not_enough_storage_space(SSD* _SSD_list, VIDEO_SEGMENT* _VIDEO_SEGMENT_list, int _to_ssd, int _from_vid) {
	return (_SSD_list[_to_ssd].storage_usage + _VIDEO_SEGMENT_list[_from_vid].size) > _SSD_list[_to_ssd].storage_capacity;
}

//c++�� split ��� ���ͳݿ��� �����ߴ� ��������....
string* split(string str, char Delimiter) {
	istringstream iss(str);             // istringstream�� str�� ��´�.
	string buffer;                      // �����ڸ� �������� ����� ���ڿ��� ������� ����
	vector<string> result;

	// istringstream�� istream�� ��ӹ����Ƿ� getline�� ����� �� �ִ�.
	while (getline(iss, buffer, Delimiter)) {
		result.push_back(buffer);               // ����� ���ڿ��� vector�� ����
	}
	
	string* result_array = new string[result.size()];
	copy(result.begin(), result.end(), result_array);
	result.clear();
	vector<string>().swap(result); //�޸� ������ ����
	return result_array;
}