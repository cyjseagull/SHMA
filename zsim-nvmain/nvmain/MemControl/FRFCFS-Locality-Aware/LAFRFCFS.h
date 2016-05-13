/*
 * 2015.8.18
 *
 */

#ifndef _LAFRFCFS_H_
#define _LAFRFCFS_H_

#include "src/MemoryController.h"
#include "include/NVMainRequest.h"
namespace NVM
{
	class LAFRFCFS : public MemoryController
	{
		public:
			LAFRFCFS();
			void SetConfig( Config* conf , bool CreateChildren=true );
			
			//issue command related
			bool IssueCommand( NVMainRequest* req );
			bool RequestComplete( NVMainRequest* req);

			void RegisterStats();
			void CalculateStats();

		protected:

		private: 
	}
};
#endif
