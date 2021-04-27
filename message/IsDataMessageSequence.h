#ifndef ISDATAMESSAGESEQUENCE_H
#define ISDATAMESSAGESEQUENCE_H

struct DataMessageSequenceMemfnsBase {
    void begin();
    void end();
    void get_cap();
    void set_cap();
    void get_data();
    void set_data();
    void get_size();
    void set_size();
    void key();
    void operator<();
};

template<typename T>
struct DataMessageSequenceMemfnsDerived
    : T, DataMessageSequenceMemfnsBase
{
};


template<typename T, T>
struct DataMessageSequenceMemfnsCheck
{
};

template<typename>
char struct DataMessageSequenceBeginHelper(...);

template <typename T>
char (&DataMessageSequenceBeginHelper(T* t, 
	typename std::enable_if<!std::is_same<
	decltype(*t.begin()), void>::value>::type*))[2]

#endif /* ISDATAMESSAGESEQUENCE_H */
