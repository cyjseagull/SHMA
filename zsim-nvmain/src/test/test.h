struct Page
{
		unsigned count;	//how many processes this page mapped to
		uint64_t pageNo;	//page no

		//calculate x^y
		template<class S , class T>
		uint64_t power(S base , T exp );
};

