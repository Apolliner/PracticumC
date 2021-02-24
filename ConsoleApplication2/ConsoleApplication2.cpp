#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber() {
	int result;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text) {
	vector<string> words;
	string word;
	for (const char c : text) {
		if (c == ' ') {
			words.push_back(word);
			word = "";
		}
		else {
			word += c;
		}
	}
	words.push_back(word);
	return words;
}

struct Document {
	int id;
	double relevance;
};

class SearchServer {
public:
	int quantitydocs = 0;
	map<int, vector<string>> textdocuments;

	void SetStopWords(const string& text) {
		for (const string& word : SplitIntoWords(text)) {
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document) {
		textdocuments[document_id] = SplitIntoWordsNoStop(document);
		++quantitydocs;
		for (const string& word : SplitIntoWordsNoStop(document)) {
			word_to_documents_[word].insert(document_id);
		}
	}

	vector<Document> FindTopDocuments(const string& query) const {
		const vector<string> query_words = SplitIntoWordsNoStop(query);
		auto matched_documents = FindAllDocuments(query);
		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs) {
				return lhs.relevance > rhs.relevance;
			}
		);
		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}
		return matched_documents;
	}


	vector<string> ProcessingMinusWords(const vector<string>& query_words) const {
		vector<string> minus_words;
		copy_if(query_words.begin(), query_words.end(), back_inserter(minus_words), []
		(const string& word) {
				return word[0] == '-';
			});
		for (int i = 0; i < minus_words.size(); ++i) {
			minus_words[i].erase(0, 1);
		}
		return minus_words;
	}

	vector<Document> BannedDocs(vector<Document>& docs, const vector<string>& minus_words) const {
		set<int> banned_docs; //создается множество забаненых документов:
		for (const string& word : minus_words) {
			if (word_to_documents_.count(word) == 0) {
				continue;
			}
			for (const int document_id : word_to_documents_.at(word)) {
				banned_docs.insert(document_id);
			}
		}
		//документы банятся:
		vector<Document> docs_no_ban;
		copy_if(docs.begin(), docs.end(), back_inserter(docs_no_ban), [&banned_docs]
		(const Document& document) {
				return !banned_docs.count(document.id);
			});

		return docs_no_ban;
	}

private:
	map<string, set<int>> word_to_documents_;
	set<string> stop_words_;

	map<string, double> IDF_relevance(const vector<string>& query) const {
		map<string, double> idf_relevance;
		for (string word : query) {
			if (word_to_documents_.count(word) == 0) {
				idf_relevance[word] = 0;
			}
			else {
				int id_docs_size = 0;
				for (auto [word1, id_docs] : word_to_documents_) {
					if (word == word1) {
						id_docs_size = id_docs.size();
					}
				}
				if (id_docs_size == 0) {
					idf_relevance[word] = 0;
				}
				else {
					idf_relevance[word] = log(static_cast<double>(quantitydocs) /
						id_docs_size);
				}
			}
		}
		return idf_relevance;
	}
	// Обределяет TF, умножает на IDF, определяет TF_IDF документа, создаёт вектор значений;
	vector<Document> TF_IDF_relevance(const vector<string>& query) const {
		map<string, double> all_idf = IDF_relevance(query);
		vector<Document> answer;
		for (auto [id, text] : textdocuments) {
			double relevance_doc = -1;
			int count_matches = 0;
			for (string word : query) {
				double tf_word = static_cast<double>(std::count(text.begin(),
					text.end(), word)) / text.size();
				relevance_doc += tf_word * all_idf[word];
				if (std::count(text.begin(), text.end(), word)) {
					++count_matches;
				}
			}
			if (count_matches > 0) {
				++relevance_doc;
			}
			answer.push_back({ id, relevance_doc });
		}

		return answer;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const {
		vector<string> words;
		for (const string& word : SplitIntoWords(text)) {
			if (stop_words_.count(word) == 0) {
				words.push_back(word);
			}
		}
		return words;
	}

	vector<Document> FindAllDocuments(const string& query) const {
		const vector<string> query_words = SplitIntoWordsNoStop(query);
		vector<Document> matched_documents = TF_IDF_relevance(query_words);
		vector<Document> docs_no_empty_relevance;
		copy_if(matched_documents.begin(), matched_documents.end(), back_inserter(docs_no_empty_relevance), []
		(const Document& document) {
				return (document.relevance >= 0);
			});
		return BannedDocs(docs_no_empty_relevance, ProcessingMinusWords(query_words));
	}
};

SearchServer CreateSearchServer() {
	SearchServer search_server;
	search_server.SetStopWords(ReadLine());
	const int document_count = ReadLineWithNumber();
	for (int document_id = 0; document_id < document_count; ++document_id) {
		search_server.AddDocument(document_id, ReadLine());
	}
	return search_server;
}


int main() {
	const SearchServer search_server = CreateSearchServer();

	const string query = ReadLine();
	for (auto [document_id, relevance] : search_server.FindTopDocuments(query)) {
		cout << "{ document_id = " << document_id << ", relevance = " << relevance << " }" << endl;
	}
}