/*
 * 2015.8.18
 *
 */
#ifndef _MANIPULATE_QUEUE_H_
#define _MANIPULATE_QUEUE_H_

namespace NVM
{
	class LocalityQueue
	{
		public:
			LocalityQueue( uint64_t queue_size , uint64_t  );
			void Upgrade();
			void Degrade();
			void GetMemoryUsage();
				

		protected:
		private:
	
	}
}

#endif
