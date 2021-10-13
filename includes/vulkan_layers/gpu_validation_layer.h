#pragma once

/*
	[[ GPU Validation ]]

	A layer which validates potentially undefined behaviour on the GPU using a JIT shader injection compilation model

	Author: Miguel Petersen
*/

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#include "structure_types.h"

#define VK_LAYER_AVA_gpu_validation_NAME "VK_LAYER_AVA_gpu_validation"
#define VK_LAYER_AVA_gpu_validation_SPEC_VERSION VK_MAKE_VERSION(1, 0, 67)
#define VK_LAYER_AVA_gpu_validation_IMPLEMENTATION_VERSION 9
#define VK_LAYER_AVA_gpu_validation_DESCRIPTION "Avalanche studios gpu validation layer"

#define VK_AVA_gpu_validation 1
#define VK_AVA_GPU_VALIDATION_TYPE "device"
#define VK_AVA_GPU_VALIDATION_SPEC_VERSION 1
#define VK_AVA_GPU_VALIDATION_EXTENSION_NAME "VK_AVA_gpu_validation"
#define VK_AVA_GPU_VALIDATION_ENTRYPOINTS vkGPUValidationCreateReportAVA,vkGPUValidationDestroyReportAVA,vkGPUValidationBeginReportAVA,vkGPUValidationGetReportStatusAVA,vkGPUValidationDrawDebugAVA,vkGPUValidationEndReportAVA,vkGPUValidationPrintReportAVA,vkGPUValidationPrintReportSummaryAVA,vkGPUValidationExportReportAVA,vkGPUValidationGetReportInfoAVA,vkGPUValidationFlushReportAVA

VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkGPUValidationObjectAVA);
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkGPUValidationReportAVA);

class IDebugRenderer;

namespace Vectormath 
{
	namespace Aos 
	{ 
		class Vector2;
	} 
}

struct VkGPUValidationShaderCreateInfoAVA
{
	VkStructureType sType;		   ///< VK_STRUCTURE_TYPE_GPU_VALIDATION_SHADER_CREATE_INFO_AVA
	const void*		pNext;		   ///< Chained structure
	const char*     m_Name;		   ///< Name of the shader
};

struct VkGPUValidationPipelineCreateInfoAVA
{
	VkStructureType sType;		   ///< VK_STRUCTURE_TYPE_GPU_VALIDATION_PIPELINE_CREATE_INFO_AVA
	const void*		pNext;		   ///< Chained structure
	const char*     m_Name;		   ///< Name of the shader
	uint32_t		m_FeatureMask; ///< Mask of features to enable on this pipeline, [NOT IMPLEMENTED]
};

enum VkGPUValidationTypeAVA
{
	VK_GPU_VALIDATION_TYPE_VALIDATION_ERROR_AVA,
	VK_GPU_VALIDATION_TYPE_REPORT_FLOW_COVERAGE_AVA,
};

enum VkGPUValidationErrorTypeAVA
{
	VK_GPU_VALIDATION_ERROR_TYPE_BUFFER_OVERFLOW_AVA,
	VK_GPU_VALIDATION_ERROR_TYPE_IMAGE_OVERFLOW_AVA,
	VK_GPU_VALIDATION_ERROR_TYPE_DESCRIPTOR_OVERFLOW_AVA,
	VK_GPU_VALIDATION_ERROR_TYPE_EXPORT_UNSTABLE,
	VK_GPU_VALIDATION_ERROR_TYPE_RESOURCE_RACE_CONDITION_AVA,
	VK_GPU_VALIDATION_ERROR_TYPE_SUBRESOURCE_UNINITIALIZED,
	VK_GPU_VALIDATION_ERROR_TYPE_COUNT,
};

struct VkGPUValidationSourceLocationAVA
{
	uint32_t m_Offset; ///< Character offset into the shader sources, UINT32_MAX denoting an invalid location
	uint32_t m_Line;
	uint32_t m_Character;
};

struct VkGPUValidationSourceSpanAVA
{
	VkGPUValidationSourceLocationAVA m_Begin;
	VkGPUValidationSourceLocationAVA m_End;
};

struct VkGPUValidationSourceExtractAVA
{
    const char*					 m_Module;	   ///< Name of the master shader object
    const char*					 m_ModuleFile; ///< File of the master module
	const char*					 m_File;	   ///< File of the offending code, may be null
	const char*					 m_Function;   ///< Name of the function in which the offending code is present, may be null
	VkGPUValidationSourceSpanAVA m_Span;	   ///< The span of the source extract, may be invalid
	const char*					 m_Extract;    ///< Source level extract of the offending code, may be null
};

struct VkGPUValidationObjectInfoAVA
{
	const char*				 m_Name;   ///< The debug name of the object
	VkGPUValidationObjectAVA m_Object; ///< The object to which this error occurred on, may be null
};

struct VkGPUValidationErrorAVA
{
	VkGPUValidationErrorTypeAVA		m_ErrorType;		///< The error type
	uint32_t						m_UserMarkerCount;	///< The number of user markers
	const char*						m_UserMarkers;		///< The user marker stack
	const char*						m_Message;			///< A customized message describing what happened
	VkGPUValidationObjectInfoAVA	m_ObjectInfo;       ///< The information of the object to which this error occurred on
	VkGPUValidationSourceExtractAVA m_SourceExtract;    ///< Source level extract of this error, may be null
};

// General flow coverage report
// Operates on the inlined instructions
struct VkGPUValidationReportFlowCoverageAVA
{
	uint64_t m_ReportUID;
	uint32_t m_InstructionCount;
	uint32_t m_InstructionCoverage;
	uint32_t m_FlowBranchCount;
	uint32_t m_FlowBranchCoverage;
};

struct VkGPUValidationMessageAVA
{
	VkGPUValidationTypeAVA m_Type;		  ///< Type of message
	uint32_t			   m_MergedCount; ///< Number of instances of this message, validation messages may be merged if equivilent
	uint32_t			   m_Feature;	  ///< The feature which reported this message

	union
	{
		VkGPUValidationErrorAVA m_Error;
		VkGPUValidationReportFlowCoverageAVA m_ReportFlowCoverage;
	};
};

struct VkGPUValidationReportCreateInfoAVA
{
	VkStructureType sType; ///< VK_STRUCTURE_TYPE_GPU_VALIDATION_REPORT_CREATE_INFO_AVA
	const void*	    pNext;	///< Chained structure
};

struct VkGPUValidationReportBeginInfoAVA
{
	VkStructureType sType;				  ///< VK_STRUCTURE_TYPE_GPU_VALIDATION_REPORT_BEGIN_INFO_AVA
	const void*	    pNext;				  ///< Chained structure
	uint32_t	    m_Features;			  ///< Enabled features, any subsequent feature requests will be masked with this
	bool			m_WaitForCompilation; ///< If true wait for all shader compilation to complete
};

struct VkGPUValidationReportInfoAVA
{
	VkGPUValidationReportAVA		 m_Report;       ///< The source report
	const VkGPUValidationMessageAVA* m_Messages;     ///< All validation messages
	uint32_t						 m_MessageCount; ///< Number of validation messages
};

enum VkGPUValidationReportStatusTypeAVA : uint8_t
{
	///< The report is currently not recording
	VK_GPU_VALIDATION_REPORT_STATUS_IDLE,

	///< The report is still waiting for pending shader compilation
	VK_GPU_VALIDATION_REPORT_STATUS_PENDING_SHADER_COMPILATION,

	///< The report is still waiting for pending pipeline compilation
	VK_GPU_VALIDATION_REPORT_STATUS_PENDING_PIPELINE_COMPILATION,
	
	///< The report is currently recording
	VK_GPU_VALIDATION_REPORT_STATUS_RECORDING,
};

struct VkGPUValidationReportStatusAVA
{
	VkGPUValidationReportStatusTypeAVA m_Type; ///< The type of this status
	union
	{
		uint32_t m_PendingShaders;			   ///< The number pending shader compilation jobs, VK_GPU_VALIDATION_REPORT_STATUS_PENDING_SHADER_COMPILATION
		uint32_t m_PendingPipelines;		   ///< The number of pending pipeline compilation jobs, VK_GPU_VALIDATION_REPORT_STATUS_PENDING_PIPELINE_COMPILATION
	};
};

enum VkGPUValidationFeatureAVA : uint32_t
{
	///< Enables shader injection for validating resource addresses
	VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS = (1 << 0),

	///< Enables shader injection for validating all relevant export operations
	VK_GPU_VALIDATION_FEATURE_SHADER_EXPORT_STABILITY = (1 << 1),

	///< Enables shader injection for validating runtime descriptor arrays
	VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS = (1 << 2),

	///< Enables shader injection for validating resource thread safety
	VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE = (1 << 3),

	///< Enables shader injection for validating resource initialization
	VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION = (1 << 4),

	///< Instrumentation Set: Basic
	///  ? Performance cost: Low
	VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_BASIC = (VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_ADDRESS_BOUNDS | VK_GPU_VALIDATION_FEATURE_SHADER_EXPORT_STABILITY | VK_GPU_VALIDATION_FEATURE_SHADER_DESCRIPTOR_ARRAY_BOUNDS),

	///< Instrumentation Set: Concurrency
	///  ? Performance cost: High, enjoy the slideshow
	VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_CONCURRENCY = (VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_DATA_RACE),

	///< Instrumentation Set: Data Residency
	///  ? Performance cost: ???
	VK_GPU_VALIDATION_FEATURE_INSTRUMENTATION_SET_DATA_RESIDENCY = (VK_GPU_VALIDATION_FEATURE_SHADER_RESOURCE_INITIALIZATION),
};

enum VkGPUValidationLogSeverity : uint8_t
{
	VK_GPU_VALIDATION_LOG_SEVERITY_INFO    = (1 << 0),
	VK_GPU_VALIDATION_LOG_SEVERITY_WARNING = (1 << 1),
	VK_GPU_VALIDATION_LOG_SEVERITY_ERROR   = (1 << 2),
};

enum VkGPUValidationReportExportFormat : uint8_t
{
	VK_GPU_VALIDATION_REPORT_EXPORT_FORMAT_CSV,
	VK_GPU_VALIDATION_REPORT_EXPORT_FORMAT_HTML,
};

///< Message callback for reports
typedef void(*VkGPUValdiationMessageCallbackAVA)(void* user_data, const VkGPUValidationReportInfoAVA* info);

///< Generic log callback
typedef void(*VkGPUValidationLogCallbackAVA)(void* user_data, VkGPUValidationLogSeverity severity, const char* file, uint32_t line, const char* message);

///< Create a new report object
typedef VkResult(VKAPI_PTR *PFN_vkGPUValidationCreateReportAVA)(VkDevice device, const VkGPUValidationReportCreateInfoAVA* create_info, VkGPUValidationReportAVA* out);

///< Destroy a report object
typedef VkResult(VKAPI_PTR *PFN_vkGPUValidationDestroyReportAVA)(VkDevice device, VkGPUValidationReportAVA report);

///< Begin recording to a report
typedef VkResult(VKAPI_PTR *PFN_vkGPUValidationBeginReportAVA)(VkDevice device, VkGPUValidationReportAVA report, const VkGPUValidationReportBeginInfoAVA* begin_info);

///< Get the status of a report
typedef VkGPUValidationReportStatusAVA(VKAPI_PTR *PFN_vkGPUValidationGetReportStatusAVA)(VkDevice device, VkGPUValidationReportAVA report);

///< Draw debug information
typedef VkResult(VKAPI_PTR *PFN_vkGPUValidationDrawDebugAVA)(VkDevice device, IDebugRenderer* renderer, const Vectormath::Aos::Vector2& position, const Vectormath::Aos::Vector2& display_size);

///< End recording to a report
typedef VkResult(VKAPI_PTR *PFN_vkGPUValidationEndReportAVA)(VkDevice device);

///< Print all queued messages within a report to the default message callback
typedef VkResult(VKAPI_PTR* PFN_vkGPUValidationPrintReportAVA)(VkDevice device, VkGPUValidationReportAVA report);

///< Print the summary of a report
typedef VkResult(VKAPI_PTR* PFN_vkGPUValidationPrintReportSummaryAVA)(VkDevice device, VkGPUValidationReportAVA report);

///< Export a report to a file stream
/// ! out invalidated on report re-export or destruction
typedef VkResult(VKAPI_PTR* PFN_vkGPUValidationExportReportAVA)(VkDevice device, VkGPUValidationReportAVA report, VkGPUValidationReportExportFormat format, const char** out);

///< Get the report info
typedef VkResult(VKAPI_PTR* PFN_vkGPUValidationGetReportInfoAVA)(VkDevice device, VkGPUValidationReportAVA report, VkGPUValidationReportInfoAVA* out);

///< Flush all queued validation messages within a report
typedef VkResult(VKAPI_PTR* PFN_vkGPUValidationFlushReportAVA)(VkDevice device, VkGPUValidationReportAVA report);

struct VkGPUValidationCreateInfoAVA
{
	VkStructureType					  sType;	  ///< VK_STRUCTURE_TYPE_GPU_VALIDATION_CREATE_INFO_AVA
	const void*						  pNext;	  ///< Chained structure

	void*							  m_UserData;        ///< Userdata supplied to callbacks
	VkGPUValdiationMessageCallbackAVA m_MessageCallback; ///< Validation message callback
	VkGPUValidationLogCallbackAVA     m_LogCallback;	 ///< Log callback
	uint8_t							  m_LogSeverityMask; ///< The mask of messages with given severity level to recieve

	bool							  m_AsyncTransfer;   ///< Set to true to enable asynchronous PCIE diagnostic data transfers
	bool							  m_LatentTransfers; ///< Set to true to reduce the PCIE load at the cost of potentially missed validation messages

	uint32_t						  m_CommandBufferMessageCountDefault; ///< The initial message count limit for any command buffer, will grow if needed to the upper limit
	uint32_t						  m_CommandBufferMessageCountLimit;	  ///< Maximum of messages that a command list may generated, further messages may overwrite previous messages
	uint32_t						  m_ChunkedWorkingSetByteSize;		  ///< The byte size of the a working set
	uint32_t						  m_ThrottleThresholdDefault;		  ///< The present lifetime threshold until which the frametime is throttled until message fitlering is complete
	uint32_t						  m_ThrottleThresholdLimit;			  ///< The present lifetime threshold until which the frametime is throttled until message fitlering is complete

	uint32_t						  m_ShaderCompilerWorkerCount;	 ///< The number of shader compiler threads
	uint32_t						  m_PipelineCompilerWorkerCount; ///< The number of pipeline compiler threads

	const char*						  m_CacheFilePath; ///< The path of the cache file, optional
	bool							  m_StripFolders;  ///< Strip folders from debug paths
};
