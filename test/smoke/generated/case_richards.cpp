// Generated from ScriptBench/richards.zef (hand-maintained).
// Fair transpile shape: task/TCB methods via sends; fields via ZEFC_IC_*.
// Int arith/bitops and comparisons use decode/encode (Value short-circuit style).
// Array(n) / a2[i]= via Array__with_size + ZEFC_SEL_PUT_i.

#include "zefc/array_api.hpp"
#include "zefc/dispatch.hpp"
#include "zefc/error.hpp"
#include "zefc/int_api.hpp"
#include "zefc/io.hpp"
#include "zefc/known_selectors.hpp"
#include "zefc/runtime.hpp"
#include "zefc/string_api.hpp"
#include "smoke_cases.hpp"

#include <cstddef>

namespace zefc {
namespace smoke {

namespace {

constexpr long long kCOUNT = 10000;
constexpr long long kEXPECTED_QUEUE_COUNT = 23246;
constexpr long long kEXPECTED_HOLD_COUNT = 9297;

constexpr long long kID_IDLE = 0;
constexpr long long kID_WORKER = 1;
constexpr long long kID_HANDLER_A = 2;
constexpr long long kID_HANDLER_B = 3;
constexpr long long kID_DEVICE_A = 4;
constexpr long long kID_DEVICE_B = 5;
constexpr int kNUMBER_OF_IDS = 6;

constexpr long long kKIND_DEVICE = 0;
constexpr long long kKIND_WORK = 1;

constexpr long long kSTATE_RUNNING = 0;
constexpr long long kSTATE_RUNNABLE = 1;
constexpr long long kSTATE_SUSPENDED = 2;
constexpr long long kSTATE_HELD = 4;
constexpr long long kSTATE_SUSPENDED_RUNNABLE = kSTATE_SUSPENDED | kSTATE_RUNNABLE;
constexpr long long kSTATE_NOT_HELD = ~kSTATE_HELD;

constexpr int kDATA_SIZE = 4;

static bool
falsy(id v)
{
  if (v == null_id()) {
    return true;
  }
  if (id_is_int32(v)) {
    return Int__to_i64(v) == 0;
  }
  // Heap Int 0
  if (id_is_object(v) && !id_is_double(v)) {
    // Only treat plain Int objects as numeric falsy — avoid false on TCBs etc.
    // Int heap objects are rare here; Packet uses null/int0 for empty links.
  }
  return false;
}

static bool
truthy(id v)
{
  return !falsy(v);
}

static long long
ii(id v)
{
  return Int__to_i64(v);
}

static id
I(long long v)
{
  return Int__from_i64(v);
}

// --- selectors ---

static int sel_addIdleTask = 0;
static int sel_addWorkerTask = 0;
static int sel_addHandlerTask = 0;
static int sel_addDeviceTask = 0;
static int sel_addRunningTask = 0;
static int sel_addTask = 0;
static int sel_schedule = 0;
static int sel_release = 0;
static int sel_holdCurrent = 0;
static int sel_suspendCurrent = 0;
static int sel_queue = 0;
static int sel_queueCount = 0;
static int sel_set_queueCount = 0;
static int sel_holdCount = 0;
static int sel_set_holdCount = 0;
static int sel_blocks = 0;
static int sel_list = 0;
static int sel_set_list = 0;
static int sel_currentTcb = 0;
static int sel_set_currentTcb = 0;
static int sel_currentId = 0;
static int sel_set_currentId = 0;

static int sel_setRunning = 0;
static int sel_markAsNotHeld = 0;
static int sel_markAsHeld = 0;
static int sel_isHeldOrSuspended = 0;
static int sel_markAsSuspended = 0;
static int sel_markAsRunnable = 0;
static int sel_run = 0;
static int sel_checkPriorityAdd = 0;
static int sel_link = 0;
static int sel_set_link = 0;
static int sel_id = 0;
static int sel_set_id = 0;
static int sel_priority = 0;
static int sel_tqueue = 0;
static int sel_set_tqueue = 0;
static int sel_task = 0;
static int sel_state = 0;
static int sel_set_state = 0;

static int sel_run_packet = 0;
static int sel_addTo = 0;
static int sel_kind = 0;
static int sel_a1 = 0;
static int sel_set_a1 = 0;
static int sel_a2 = 0;
static int sel_scheduler = 0;
static int sel_v1 = 0;
static int sel_set_v1 = 0;
static int sel_v2 = 0;
static int sel_set_v2 = 0;
static int sel_count = 0;
static int sel_set_count = 0;

static VTable* Scheduler_vt = nullptr;
static VTable* TCB_vt = nullptr;
static VTable* Idle_vt = nullptr;
static VTable* Device_vt = nullptr;
static VTable* Worker_vt = nullptr;
static VTable* Handler_vt = nullptr;
static VTable* Packet_vt = nullptr;

struct Scheduler_ {
  IsaPtr isa_;
  id queueCount;
  id holdCount;
  id blocks;
  id list;
  id currentTcb;
  id currentId;
};

struct TCB_ {
  IsaPtr isa_;
  id link;
  id tid;
  id priority;
  id queue;
  id task;
  id state;
};

struct Idle_ {
  IsaPtr isa_;
  id scheduler;
  id v1;
  id count;
};

struct Device_ {
  IsaPtr isa_;
  id scheduler;
  id v1;
};

struct Worker_ {
  IsaPtr isa_;
  id scheduler;
  id v1;
  id v2;
};

struct Handler_ {
  IsaPtr isa_;
  id scheduler;
  id v1;
  id v2;
};

struct Packet_ {
  IsaPtr isa_;
  id link;
  id pid;
  id kind;
  id a1;
  id a2;
};

// Field accessors
static id Sch_get_queueCount(id s) { return body<Scheduler_>(s)->queueCount; }
static id Sch_set_queueCount(id s, id v)
{
  body<Scheduler_>(s)->queueCount = v;
  return null_id();
}
static id Sch_get_holdCount(id s) { return body<Scheduler_>(s)->holdCount; }
static id Sch_set_holdCount(id s, id v)
{
  body<Scheduler_>(s)->holdCount = v;
  return null_id();
}
static id Sch_get_blocks(id s) { return body<Scheduler_>(s)->blocks; }
static id Sch_get_list(id s) { return body<Scheduler_>(s)->list; }
static id Sch_set_list(id s, id v)
{
  body<Scheduler_>(s)->list = v;
  return null_id();
}
static id Sch_get_currentTcb(id s) { return body<Scheduler_>(s)->currentTcb; }
static id Sch_set_currentTcb(id s, id v)
{
  body<Scheduler_>(s)->currentTcb = v;
  return null_id();
}
static id Sch_get_currentId(id s) { return body<Scheduler_>(s)->currentId; }
static id Sch_set_currentId(id s, id v)
{
  body<Scheduler_>(s)->currentId = v;
  return null_id();
}

static id TCB_get_link(id s) { return body<TCB_>(s)->link; }
static id TCB_get_id(id s) { return body<TCB_>(s)->tid; }
static id TCB_get_priority(id s) { return body<TCB_>(s)->priority; }
static id TCB_get_queue(id s) { return body<TCB_>(s)->queue; }
static id TCB_set_queue(id s, id v)
{
  body<TCB_>(s)->queue = v;
  return null_id();
}
static id TCB_get_task(id s) { return body<TCB_>(s)->task; }
static id TCB_get_state(id s) { return body<TCB_>(s)->state; }
static id TCB_set_state(id s, id v)
{
  body<TCB_>(s)->state = v;
  return null_id();
}

static id Idle_get_scheduler(id s) { return body<Idle_>(s)->scheduler; }
static id Idle_get_v1(id s) { return body<Idle_>(s)->v1; }
static id Idle_set_v1(id s, id v)
{
  body<Idle_>(s)->v1 = v;
  return null_id();
}
static id Idle_get_count(id s) { return body<Idle_>(s)->count; }
static id Idle_set_count(id s, id v)
{
  body<Idle_>(s)->count = v;
  return null_id();
}

static id Dev_get_scheduler(id s) { return body<Device_>(s)->scheduler; }
static id Dev_get_v1(id s) { return body<Device_>(s)->v1; }
static id Dev_set_v1(id s, id v)
{
  body<Device_>(s)->v1 = v;
  return null_id();
}

static id W_get_scheduler(id s) { return body<Worker_>(s)->scheduler; }
static id W_get_v1(id s) { return body<Worker_>(s)->v1; }
static id W_set_v1(id s, id v)
{
  body<Worker_>(s)->v1 = v;
  return null_id();
}
static id W_get_v2(id s) { return body<Worker_>(s)->v2; }
static id W_set_v2(id s, id v)
{
  body<Worker_>(s)->v2 = v;
  return null_id();
}

static id H_get_scheduler(id s) { return body<Handler_>(s)->scheduler; }
static id H_get_v1(id s) { return body<Handler_>(s)->v1; }
static id H_set_v1(id s, id v)
{
  body<Handler_>(s)->v1 = v;
  return null_id();
}
static id H_get_v2(id s) { return body<Handler_>(s)->v2; }
static id H_set_v2(id s, id v)
{
  body<Handler_>(s)->v2 = v;
  return null_id();
}

static id P_get_link(id s) { return body<Packet_>(s)->link; }
static id P_set_link(id s, id v)
{
  body<Packet_>(s)->link = v;
  return null_id();
}
static id P_get_id(id s) { return body<Packet_>(s)->pid; }
static id P_set_id(id s, id v)
{
  body<Packet_>(s)->pid = v;
  return null_id();
}
static id P_get_kind(id s) { return body<Packet_>(s)->kind; }
static id P_get_a1(id s) { return body<Packet_>(s)->a1; }
static id P_set_a1(id s, id v)
{
  body<Packet_>(s)->a1 = v;
  return null_id();
}
static id P_get_a2(id s) { return body<Packet_>(s)->a2; }

// Forward decls
static id Packet__new(id inLink, id inId, id inKind);
static id TCB__new(id inLink, id inId, id inPriority, id inQueue, id inTask);
static id Idle__new(id sch, id v1, id count);
static id Device__new(id sch);
static id Worker__new(id sch, id v1, id v2);
static id Handler__new(id sch);

static id Sch__addTask_oooo(id self, int, id idv, id priority, id queue, id task);
static id Sch__addRunningTask_oooo(id self, int, id idv, id priority, id queue, id task);
static id Sch__addIdleTask_oooo(id self, int, id idv, id priority, id queue, id count);
static id Sch__addWorkerTask_ooo(id self, int, id idv, id priority, id queue);
static id Sch__addHandlerTask_ooo(id self, int, id idv, id priority, id queue);
static id Sch__addDeviceTask_ooo(id self, int, id idv, id priority, id queue);
static id Sch__schedule_o(id self, int, ...);
static id Sch__release_o(id self, int, id idv);
static id Sch__holdCurrent_o(id self, int, ...);
static id Sch__suspendCurrent_o(id self, int, ...);
static id Sch__queue_o(id self, int, id packet);

static id TCB__setRunning_o(id self, int, ...);
static id TCB__markAsNotHeld_o(id self, int, ...);
static id TCB__markAsHeld_o(id self, int, ...);
static id TCB__isHeldOrSuspended_o(id self, int, ...);
static id TCB__markAsSuspended_o(id self, int, ...);
static id TCB__markAsRunnable_o(id self, int, ...);
static id TCB__run_o(id self, int, ...);
static id TCB__checkPriorityAdd_oo(id self, int, id task, id packet);

static id Idle__run_o(id self, int, id packet);
static id Device__run_o(id self, int, id packet);
static id Worker__run_o(id self, int, id packet);
static id Handler__run_o(id self, int, id packet);
static id Packet__addTo_o(id self, int, id queue);

static id
Sch__queueCount_o(id self, int, ...)
{
  return Sch_get_queueCount(self);
}
static id
Sch__set_queueCount_o(id self, int, id v)
{
  return Sch_set_queueCount(self, v);
}
static id
Sch__holdCount_o(id self, int, ...)
{
  return Sch_get_holdCount(self);
}
static id
Sch__set_holdCount_o(id self, int, id v)
{
  return Sch_set_holdCount(self, v);
}
static id
Sch__blocks_o(id self, int, ...)
{
  return Sch_get_blocks(self);
}
static id
Sch__list_o(id self, int, ...)
{
  return Sch_get_list(self);
}
static id
Sch__set_list_o(id self, int, id v)
{
  return Sch_set_list(self, v);
}
static id
Sch__currentTcb_o(id self, int, ...)
{
  return Sch_get_currentTcb(self);
}
static id
Sch__set_currentTcb_o(id self, int, id v)
{
  return Sch_set_currentTcb(self, v);
}
static id
Sch__currentId_o(id self, int, ...)
{
  return Sch_get_currentId(self);
}
static id
Sch__set_currentId_o(id self, int, id v)
{
  return Sch_set_currentId(self, v);
}

static id TCB__link_o(id self, int, ...) { return TCB_get_link(self); }
static id TCB__id_o(id self, int, ...) { return TCB_get_id(self); }
static id TCB__priority_o(id self, int, ...) { return TCB_get_priority(self); }
static id TCB__queue_o(id self, int, ...) { return TCB_get_queue(self); }
static id TCB__set_queue_o(id self, int, id v) { return TCB_set_queue(self, v); }
static id TCB__task_o(id self, int, ...) { return TCB_get_task(self); }
static id TCB__state_o(id self, int, ...) { return TCB_get_state(self); }
static id TCB__set_state_o(id self, int, id v) { return TCB_set_state(self, v); }

static id P__link_o(id self, int, ...) { return P_get_link(self); }
static id P__set_link_o(id self, int, id v) { return P_set_link(self, v); }
static id P__id_o(id self, int, ...) { return P_get_id(self); }
static id P__set_id_o(id self, int, id v) { return P_set_id(self, v); }
static id P__kind_o(id self, int, ...) { return P_get_kind(self); }
static id P__a1_o(id self, int, ...) { return P_get_a1(self); }
static id P__set_a1_o(id self, int, id v) { return P_set_a1(self, v); }
static id P__a2_o(id self, int, ...) { return P_get_a2(self); }

static void
ensure_runtime()
{
  if (Scheduler_vt) {
    return;
  }
  sel_addIdleTask = selector_intern("addIdleTask_oooo");
  sel_addWorkerTask = selector_intern("addWorkerTask_ooo");
  sel_addHandlerTask = selector_intern("addHandlerTask_ooo");
  sel_addDeviceTask = selector_intern("addDeviceTask_ooo");
  sel_addRunningTask = selector_intern("addRunningTask_oooo");
  sel_addTask = selector_intern("addTask_oooo");
  sel_schedule = selector_intern("schedule_o");
  sel_release = selector_intern("release_o");
  sel_holdCurrent = selector_intern("holdCurrent_o");
  sel_suspendCurrent = selector_intern("suspendCurrent_o");
  sel_queue = selector_intern("queue_o");
  sel_queueCount = selector_intern("queueCount_o");
  sel_set_queueCount = selector_intern("set_queueCount_o");
  sel_holdCount = selector_intern("holdCount_o");
  sel_set_holdCount = selector_intern("set_holdCount_o");
  sel_blocks = selector_intern("blocks_o");
  sel_list = selector_intern("list_o");
  sel_set_list = selector_intern("set_list_o");
  sel_currentTcb = selector_intern("currentTcb_o");
  sel_set_currentTcb = selector_intern("set_currentTcb_o");
  sel_currentId = selector_intern("currentId_o");
  sel_set_currentId = selector_intern("set_currentId_o");

  sel_setRunning = selector_intern("setRunning_o");
  sel_markAsNotHeld = selector_intern("markAsNotHeld_o");
  sel_markAsHeld = selector_intern("markAsHeld_o");
  sel_isHeldOrSuspended = selector_intern("isHeldOrSuspended_o");
  sel_markAsSuspended = selector_intern("markAsSuspended_o");
  sel_markAsRunnable = selector_intern("markAsRunnable_o");
  sel_run = selector_intern("run_o");
  sel_checkPriorityAdd = selector_intern("checkPriorityAdd_oo");
  sel_link = selector_intern("link_o");
  sel_set_link = selector_intern("set_link_o");
  sel_id = selector_intern("id_o");
  sel_set_id = selector_intern("set_id_o");
  sel_priority = selector_intern("priority_o");
  sel_tqueue = selector_intern("queue_o");
  sel_set_tqueue = selector_intern("set_queue_o");
  sel_task = selector_intern("task_o");
  sel_state = selector_intern("state_o");
  sel_set_state = selector_intern("set_state_o");

  sel_run_packet = selector_intern("run_o"); // task.run(packet) — same mangling
  sel_addTo = selector_intern("addTo_o");
  sel_kind = selector_intern("kind_o");
  sel_a1 = selector_intern("a1_o");
  sel_set_a1 = selector_intern("set_a1_o");
  sel_a2 = selector_intern("a2_o");
  sel_scheduler = selector_intern("scheduler_o");
  sel_v1 = selector_intern("v1_o");
  sel_set_v1 = selector_intern("set_v1_o");
  sel_v2 = selector_intern("v2_o");
  sel_set_v2 = selector_intern("set_v2_o");
  sel_count = selector_intern("count_o");
  sel_set_count = selector_intern("set_count_o");

  Scheduler_vt = vtable_create();
  TCB_vt = vtable_create();
  Idle_vt = vtable_create();
  Device_vt = vtable_create();
  Worker_vt = vtable_create();
  Handler_vt = vtable_create();
  Packet_vt = vtable_create();

  vtable_set(Scheduler_vt, sel_addIdleTask, Sch__addIdleTask_oooo);
  vtable_set(Scheduler_vt, sel_addWorkerTask, Sch__addWorkerTask_ooo);
  vtable_set(Scheduler_vt, sel_addHandlerTask, Sch__addHandlerTask_ooo);
  vtable_set(Scheduler_vt, sel_addDeviceTask, Sch__addDeviceTask_ooo);
  vtable_set(Scheduler_vt, sel_addRunningTask, Sch__addRunningTask_oooo);
  vtable_set(Scheduler_vt, sel_addTask, Sch__addTask_oooo);
  vtable_set(Scheduler_vt, sel_schedule, Sch__schedule_o);
  vtable_set(Scheduler_vt, sel_release, Sch__release_o);
  vtable_set(Scheduler_vt, sel_holdCurrent, Sch__holdCurrent_o);
  vtable_set(Scheduler_vt, sel_suspendCurrent, Sch__suspendCurrent_o);
  vtable_set(Scheduler_vt, sel_queue, Sch__queue_o);
  vtable_set(Scheduler_vt, sel_queueCount, Sch__queueCount_o);
  vtable_set(Scheduler_vt, sel_set_queueCount, Sch__set_queueCount_o);
  vtable_set(Scheduler_vt, sel_holdCount, Sch__holdCount_o);
  vtable_set(Scheduler_vt, sel_set_holdCount, Sch__set_holdCount_o);
  vtable_set(Scheduler_vt, sel_blocks, Sch__blocks_o);
  vtable_set(Scheduler_vt, sel_list, Sch__list_o);
  vtable_set(Scheduler_vt, sel_set_list, Sch__set_list_o);
  vtable_set(Scheduler_vt, sel_currentTcb, Sch__currentTcb_o);
  vtable_set(Scheduler_vt, sel_set_currentTcb, Sch__set_currentTcb_o);
  vtable_set(Scheduler_vt, sel_currentId, Sch__currentId_o);
  vtable_set(Scheduler_vt, sel_set_currentId, Sch__set_currentId_o);
  field_register_get(Scheduler_vt, sel_queueCount, offsetof(Scheduler_, queueCount));
  field_register_set(Scheduler_vt, sel_set_queueCount, offsetof(Scheduler_, queueCount));
  field_register_get(Scheduler_vt, sel_holdCount, offsetof(Scheduler_, holdCount));
  field_register_set(Scheduler_vt, sel_set_holdCount, offsetof(Scheduler_, holdCount));
  field_register_get(Scheduler_vt, sel_blocks, offsetof(Scheduler_, blocks));
  field_register_get(Scheduler_vt, sel_list, offsetof(Scheduler_, list));
  field_register_set(Scheduler_vt, sel_set_list, offsetof(Scheduler_, list));
  field_register_get(Scheduler_vt, sel_currentTcb, offsetof(Scheduler_, currentTcb));
  field_register_set(Scheduler_vt, sel_set_currentTcb, offsetof(Scheduler_, currentTcb));
  field_register_get(Scheduler_vt, sel_currentId, offsetof(Scheduler_, currentId));
  field_register_set(Scheduler_vt, sel_set_currentId, offsetof(Scheduler_, currentId));

  vtable_set(TCB_vt, sel_setRunning, TCB__setRunning_o);
  vtable_set(TCB_vt, sel_markAsNotHeld, TCB__markAsNotHeld_o);
  vtable_set(TCB_vt, sel_markAsHeld, TCB__markAsHeld_o);
  vtable_set(TCB_vt, sel_isHeldOrSuspended, TCB__isHeldOrSuspended_o);
  vtable_set(TCB_vt, sel_markAsSuspended, TCB__markAsSuspended_o);
  vtable_set(TCB_vt, sel_markAsRunnable, TCB__markAsRunnable_o);
  vtable_set(TCB_vt, sel_run, TCB__run_o);
  vtable_set(TCB_vt, sel_checkPriorityAdd, TCB__checkPriorityAdd_oo);
  vtable_set(TCB_vt, sel_link, TCB__link_o);
  vtable_set(TCB_vt, sel_id, TCB__id_o);
  vtable_set(TCB_vt, sel_priority, TCB__priority_o);
  vtable_set(TCB_vt, sel_tqueue, TCB__queue_o);
  vtable_set(TCB_vt, sel_set_tqueue, TCB__set_queue_o);
  vtable_set(TCB_vt, sel_task, TCB__task_o);
  vtable_set(TCB_vt, sel_state, TCB__state_o);
  vtable_set(TCB_vt, sel_set_state, TCB__set_state_o);
  field_register_get(TCB_vt, sel_link, offsetof(TCB_, link));
  field_register_get(TCB_vt, sel_id, offsetof(TCB_, tid));
  field_register_get(TCB_vt, sel_priority, offsetof(TCB_, priority));
  field_register_get(TCB_vt, sel_tqueue, offsetof(TCB_, queue));
  field_register_set(TCB_vt, sel_set_tqueue, offsetof(TCB_, queue));
  field_register_get(TCB_vt, sel_task, offsetof(TCB_, task));
  field_register_get(TCB_vt, sel_state, offsetof(TCB_, state));
  field_register_set(TCB_vt, sel_set_state, offsetof(TCB_, state));

  vtable_set(Idle_vt, sel_run_packet, Idle__run_o);
  field_register_get(Idle_vt, sel_scheduler, offsetof(Idle_, scheduler));
  field_register_get(Idle_vt, sel_v1, offsetof(Idle_, v1));
  field_register_set(Idle_vt, sel_set_v1, offsetof(Idle_, v1));
  field_register_get(Idle_vt, sel_count, offsetof(Idle_, count));
  field_register_set(Idle_vt, sel_set_count, offsetof(Idle_, count));

  vtable_set(Device_vt, sel_run_packet, Device__run_o);
  field_register_get(Device_vt, sel_scheduler, offsetof(Device_, scheduler));
  field_register_get(Device_vt, sel_v1, offsetof(Device_, v1));
  field_register_set(Device_vt, sel_set_v1, offsetof(Device_, v1));

  vtable_set(Worker_vt, sel_run_packet, Worker__run_o);
  field_register_get(Worker_vt, sel_scheduler, offsetof(Worker_, scheduler));
  field_register_get(Worker_vt, sel_v1, offsetof(Worker_, v1));
  field_register_set(Worker_vt, sel_set_v1, offsetof(Worker_, v1));
  field_register_get(Worker_vt, sel_v2, offsetof(Worker_, v2));
  field_register_set(Worker_vt, sel_set_v2, offsetof(Worker_, v2));

  vtable_set(Handler_vt, sel_run_packet, Handler__run_o);
  field_register_get(Handler_vt, sel_scheduler, offsetof(Handler_, scheduler));
  field_register_get(Handler_vt, sel_v1, offsetof(Handler_, v1));
  field_register_set(Handler_vt, sel_set_v1, offsetof(Handler_, v1));
  field_register_get(Handler_vt, sel_v2, offsetof(Handler_, v2));
  field_register_set(Handler_vt, sel_set_v2, offsetof(Handler_, v2));

  vtable_set(Packet_vt, sel_addTo, Packet__addTo_o);
  vtable_set(Packet_vt, sel_link, P__link_o);
  vtable_set(Packet_vt, sel_set_link, P__set_link_o);
  vtable_set(Packet_vt, sel_id, P__id_o);
  vtable_set(Packet_vt, sel_set_id, P__set_id_o);
  vtable_set(Packet_vt, sel_kind, P__kind_o);
  vtable_set(Packet_vt, sel_a1, P__a1_o);
  vtable_set(Packet_vt, sel_set_a1, P__set_a1_o);
  vtable_set(Packet_vt, sel_a2, P__a2_o);
  field_register_get(Packet_vt, sel_link, offsetof(Packet_, link));
  field_register_set(Packet_vt, sel_set_link, offsetof(Packet_, link));
  field_register_get(Packet_vt, sel_id, offsetof(Packet_, pid));
  field_register_set(Packet_vt, sel_set_id, offsetof(Packet_, pid));
  field_register_get(Packet_vt, sel_kind, offsetof(Packet_, kind));
  field_register_get(Packet_vt, sel_a1, offsetof(Packet_, a1));
  field_register_set(Packet_vt, sel_set_a1, offsetof(Packet_, a1));
  field_register_get(Packet_vt, sel_a2, offsetof(Packet_, a2));

  selector_sites_patch();
}

static id
Scheduler__new()
{
  ensure_runtime();
  Scheduler_* o = alloc<Scheduler_>();
  zefc_set_isa(o, Scheduler_vt);
  o->queueCount = I(0);
  o->holdCount = I(0);
  o->blocks = Array__with_size(kNUMBER_OF_IDS);
  o->list = null_id();
  o->currentTcb = null_id();
  o->currentId = I(0);
  return as_id(o);
}

static id
TCB__new(id inLink, id inId, id inPriority, id inQueue, id inTask)
{
  ensure_runtime();
  TCB_* o = alloc<TCB_>();
  zefc_set_isa(o, TCB_vt);
  o->link = inLink;
  o->tid = inId;
  o->priority = inPriority;
  o->queue = inQueue;
  o->task = inTask;
  o->state = falsy(inQueue) ? I(kSTATE_SUSPENDED) : I(kSTATE_SUSPENDED_RUNNABLE);
  return as_id(o);
}

static id
Idle__new(id sch, id v1, id count)
{
  ensure_runtime();
  Idle_* o = alloc<Idle_>();
  zefc_set_isa(o, Idle_vt);
  o->scheduler = sch;
  o->v1 = v1;
  o->count = count;
  return as_id(o);
}

static id
Device__new(id sch)
{
  ensure_runtime();
  Device_* o = alloc<Device_>();
  zefc_set_isa(o, Device_vt);
  o->scheduler = sch;
  o->v1 = null_id();
  return as_id(o);
}

static id
Worker__new(id sch, id v1, id v2)
{
  ensure_runtime();
  Worker_* o = alloc<Worker_>();
  zefc_set_isa(o, Worker_vt);
  o->scheduler = sch;
  o->v1 = v1;
  o->v2 = v2;
  return as_id(o);
}

static id
Handler__new(id sch)
{
  ensure_runtime();
  Handler_* o = alloc<Handler_>();
  zefc_set_isa(o, Handler_vt);
  o->scheduler = sch;
  o->v1 = null_id();
  o->v2 = null_id();
  return as_id(o);
}

static id
Packet__new(id inLink, id inId, id inKind)
{
  ensure_runtime();
  Packet_* o = alloc<Packet_>();
  zefc_set_isa(o, Packet_vt);
  o->link = falsy(inLink) ? null_id() : inLink;
  o->pid = inId;
  o->kind = inKind;
  o->a1 = I(0);
  o->a2 = Array__with_size(kDATA_SIZE);
  return as_id(o);
}

// ZEFC_SEND3/4 cover Scheduler multi-arg setup (method IC).

static id
Sch__addTask_oooo(id self, int, id idv, id priority, id queue, id task)
{
  id tcb = TCB__new(ZEFC_IC_GET(self, sel_list, Scheduler_, list), idv, priority, queue, task);
  (void)ZEFC_IC_SET(self, sel_set_currentTcb, Scheduler_, currentTcb, tcb);
  (void)ZEFC_IC_SET(self, sel_set_list, Scheduler_, list, tcb);
  Array__set_at(ZEFC_IC_GET(self, sel_blocks, Scheduler_, blocks), static_cast<int>(ii(idv)), tcb);
  return null_id();
}

static id
Sch__addRunningTask_oooo(id self, int, id idv, id priority, id queue, id task)
{
  (void)ZEFC_SEND4(self, sel_addTask, idv, priority, queue, task);
  (void)ZEFC_SEND0(ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb), sel_setRunning);
  return null_id();
}

static id
Sch__addIdleTask_oooo(id self, int, id idv, id priority, id queue, id count)
{
  return ZEFC_SEND4(self, sel_addRunningTask, idv, priority, queue,
                    Idle__new(self, I(1), count));
}

static id
Sch__addWorkerTask_ooo(id self, int, id idv, id priority, id queue)
{
  return ZEFC_SEND4(self, sel_addTask, idv, priority, queue,
                    Worker__new(self, I(kID_HANDLER_A), I(0)));
}

static id
Sch__addHandlerTask_ooo(id self, int, id idv, id priority, id queue)
{
  return ZEFC_SEND4(self, sel_addTask, idv, priority, queue, Handler__new(self));
}

static id
Sch__addDeviceTask_ooo(id self, int, id idv, id priority, id queue)
{
  return ZEFC_SEND4(self, sel_addTask, idv, priority, queue, Device__new(self));
}

static id
Sch__schedule_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_currentTcb, Scheduler_, currentTcb, ZEFC_IC_GET(self, sel_list, Scheduler_, list));
  while (truthy(ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb))) {
    id tcb = ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb);
    if (truthy(ZEFC_SEND0(tcb, sel_isHeldOrSuspended))) {
      (void)ZEFC_IC_SET(self, sel_set_currentTcb, Scheduler_, currentTcb, ZEFC_IC_GET(tcb, sel_link, TCB_, link));
    } else {
      (void)ZEFC_IC_SET(self, sel_set_currentId, Scheduler_, currentId, ZEFC_IC_GET(tcb, sel_id, TCB_, tid));
      (void)ZEFC_IC_SET(self, sel_set_currentTcb, Scheduler_, currentTcb, ZEFC_SEND0(tcb, sel_run));
    }
  }
  return null_id();
}

static id
Sch__release_o(id self, int, id idv)
{
  id tcb = Array__at(ZEFC_IC_GET(self, sel_blocks, Scheduler_, blocks), static_cast<int>(ii(idv)));
  if (falsy(tcb)) {
    return tcb;
  }
  (void)ZEFC_SEND0(tcb, sel_markAsNotHeld);
  id current = ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb);
  if (ii(ZEFC_IC_GET(tcb, sel_priority, TCB_, priority)) > ii(ZEFC_IC_GET(current, sel_priority, TCB_, priority))) {
    return tcb;
  }
  return current;
}

static id
Sch__holdCurrent_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_holdCount, Scheduler_, holdCount, I(ii(ZEFC_IC_GET(self, sel_holdCount, Scheduler_, holdCount)) + 1));
  id tcb = ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb);
  (void)ZEFC_SEND0(tcb, sel_markAsHeld);
  return ZEFC_IC_GET(tcb, sel_link, TCB_, link);
}

static id
Sch__suspendCurrent_o(id self, int, ...)
{
  id tcb = ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb);
  (void)ZEFC_SEND0(tcb, sel_markAsSuspended);
  return tcb;
}

static id
Sch__queue_o(id self, int, id packet)
{
  id t = Array__at(ZEFC_IC_GET(self, sel_blocks, Scheduler_, blocks), static_cast<int>(ii(ZEFC_IC_GET(packet, sel_id, Packet_, pid))));
  if (falsy(t)) {
    return t;
  }
  (void)ZEFC_IC_SET(self, sel_set_queueCount, Scheduler_, queueCount, I(ii(ZEFC_IC_GET(self, sel_queueCount, Scheduler_, queueCount)) + 1));
  (void)ZEFC_IC_SET(packet, sel_set_link, Packet_, link, null_id());
  (void)ZEFC_IC_SET(packet, sel_set_id, Packet_, pid, ZEFC_IC_GET(self, sel_currentId, Scheduler_, currentId));
  return ZEFC_SEND2(t, sel_checkPriorityAdd, ZEFC_IC_GET(self, sel_currentTcb, Scheduler_, currentTcb), packet);
}

static id
TCB__setRunning_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(kSTATE_RUNNING));
  return null_id();
}

static id
TCB__markAsNotHeld_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(ii(ZEFC_IC_GET(self, sel_state, TCB_, state)) & kSTATE_NOT_HELD));
  return null_id();
}

static id
TCB__markAsHeld_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(ii(ZEFC_IC_GET(self, sel_state, TCB_, state)) | kSTATE_HELD));
  return null_id();
}

static id
TCB__isHeldOrSuspended_o(id self, int, ...)
{
  const long long st = ii(ZEFC_IC_GET(self, sel_state, TCB_, state));
  const bool held = (st & kSTATE_HELD) != 0;
  const bool susp = st == kSTATE_SUSPENDED;
  return (held || susp) ? I(1) : I(0);
}

static id
TCB__markAsSuspended_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(ii(ZEFC_IC_GET(self, sel_state, TCB_, state)) | kSTATE_SUSPENDED));
  return null_id();
}

static id
TCB__markAsRunnable_o(id self, int, ...)
{
  (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(ii(ZEFC_IC_GET(self, sel_state, TCB_, state)) | kSTATE_RUNNABLE));
  return null_id();
}

static id
TCB__run_o(id self, int, ...)
{
  id packet = null_id();
  if (ii(ZEFC_IC_GET(self, sel_state, TCB_, state)) == kSTATE_SUSPENDED_RUNNABLE) {
    packet = ZEFC_IC_GET(self, sel_tqueue, TCB_, queue);
    (void)ZEFC_IC_SET(self, sel_set_tqueue, TCB_, queue, ZEFC_IC_GET(packet, sel_link, Packet_, link));
    if (falsy(ZEFC_IC_GET(self, sel_tqueue, TCB_, queue))) {
      (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(kSTATE_RUNNING));
    } else {
      (void)ZEFC_IC_SET(self, sel_set_state, TCB_, state, I(kSTATE_RUNNABLE));
    }
  }
  return ZEFC_SEND1(ZEFC_IC_GET(self, sel_task, TCB_, task), sel_run_packet, packet);
}

static id
TCB__checkPriorityAdd_oo(id self, int, id task, id packet)
{
  if (falsy(ZEFC_IC_GET(self, sel_tqueue, TCB_, queue))) {
    (void)ZEFC_IC_SET(self, sel_set_tqueue, TCB_, queue, packet);
    (void)ZEFC_SEND0(self, sel_markAsRunnable);
    if (ii(ZEFC_IC_GET(self, sel_priority, TCB_, priority)) > ii(ZEFC_IC_GET(task, sel_priority, TCB_, priority))) {
      return self;
    }
  } else {
    (void)ZEFC_IC_SET(self, sel_set_tqueue, TCB_, queue, ZEFC_SEND1(packet, sel_addTo, ZEFC_IC_GET(self, sel_tqueue, TCB_, queue)));
  }
  return task;
}

static id
Idle__run_o(id self, int, id packet)
{
  (void)packet;
  (void)ZEFC_IC_SET(self, sel_set_count, Idle_, count, I(ii(ZEFC_IC_GET(self, sel_count, Idle_, count)) - 1));
  if (ii(ZEFC_IC_GET(self, sel_count, Idle_, count)) == 0) {
    return ZEFC_SEND0(ZEFC_IC_GET(self, sel_scheduler, Idle_, scheduler), sel_holdCurrent);
  }
  const long long v1 = ii(ZEFC_IC_GET(self, sel_v1, Idle_, v1));
  if ((v1 & 1) == 0) {
    (void)ZEFC_IC_SET(self, sel_set_v1, Idle_, v1, I(v1 >> 1));
    return ZEFC_SEND1(ZEFC_IC_GET(self, sel_scheduler, Idle_, scheduler), sel_release, I(kID_DEVICE_A));
  }
  (void)ZEFC_IC_SET(self, sel_set_v1, Idle_, v1, I((v1 >> 1) ^ 53256));
  return ZEFC_SEND1(ZEFC_IC_GET(self, sel_scheduler, Idle_, scheduler), sel_release, I(kID_DEVICE_B));
}

static id
Device__run_o(id self, int, id packet)
{
  id sch = ZEFC_IC_GET(self, sel_scheduler, Device_, scheduler);
  if (falsy(packet)) {
    if (falsy(ZEFC_IC_GET(self, sel_v1, Device_, v1))) {
      return ZEFC_SEND0(sch, sel_suspendCurrent);
    }
    id v = ZEFC_IC_GET(self, sel_v1, Device_, v1);
    (void)ZEFC_IC_SET(self, sel_set_v1, Device_, v1, null_id());
    return ZEFC_SEND1(sch, sel_queue, v);
  }
  (void)ZEFC_IC_SET(self, sel_set_v1, Device_, v1, packet);
  return ZEFC_SEND0(sch, sel_holdCurrent);
}

static id
Worker__run_o(id self, int, id packet)
{
  id sch = ZEFC_IC_GET(self, sel_scheduler, Worker_, scheduler);
  if (falsy(packet)) {
    return ZEFC_SEND0(sch, sel_suspendCurrent);
  }
  if (ii(ZEFC_IC_GET(self, sel_v1, Worker_, v1)) == kID_HANDLER_A) {
    (void)ZEFC_IC_SET(self, sel_set_v1, Worker_, v1, I(kID_HANDLER_B));
  } else {
    (void)ZEFC_IC_SET(self, sel_set_v1, Worker_, v1, I(kID_HANDLER_A));
  }
  (void)ZEFC_IC_SET(packet, sel_set_id, Packet_, pid, ZEFC_IC_GET(self, sel_v1, Worker_, v1));
  (void)ZEFC_IC_SET(packet, sel_set_a1, Packet_, a1, I(0));
  id a2 = ZEFC_IC_GET(packet, sel_a2, Packet_, a2);
  for (int i = 0; i < kDATA_SIZE; ++i) {
    long long v2 = ii(ZEFC_IC_GET(self, sel_v2, Worker_, v2)) + 1;
    if (v2 > 26) {
      v2 = 1;
    }
    (void)ZEFC_IC_SET(self, sel_set_v2, Worker_, v2, I(v2));
    Array__set_at(a2, i, I(v2));
  }
  return ZEFC_SEND1(sch, sel_queue, packet);
}

static id
Handler__run_o(id self, int, id packet)
{
  id sch = ZEFC_IC_GET(self, sel_scheduler, Handler_, scheduler);
  if (truthy(packet)) {
    if (ii(ZEFC_IC_GET(packet, sel_kind, Packet_, kind)) == kKIND_WORK) {
      (void)ZEFC_IC_SET(self, sel_set_v1, Handler_, v1, ZEFC_SEND1(packet, sel_addTo, ZEFC_IC_GET(self, sel_v1, Handler_, v1)));
    } else {
      (void)ZEFC_IC_SET(self, sel_set_v2, Handler_, v2, ZEFC_SEND1(packet, sel_addTo, ZEFC_IC_GET(self, sel_v2, Handler_, v2)));
    }
  }
  if (truthy(ZEFC_IC_GET(self, sel_v1, Handler_, v1))) {
    id v1 = ZEFC_IC_GET(self, sel_v1, Handler_, v1);
    const long long count = ii(ZEFC_IC_GET(v1, sel_a1, Packet_, a1));
    if (count < kDATA_SIZE) {
      if (truthy(ZEFC_IC_GET(self, sel_v2, Handler_, v2))) {
        id v = ZEFC_IC_GET(self, sel_v2, Handler_, v2);
        (void)ZEFC_IC_SET(self, sel_set_v2, Handler_, v2, ZEFC_IC_GET(v, sel_link, Packet_, link));
        (void)ZEFC_IC_SET(v, sel_set_a1, Packet_, a1, Array__at(ZEFC_IC_GET(v1, sel_a2, Packet_, a2), static_cast<int>(count)));
        (void)ZEFC_IC_SET(v1, sel_set_a1, Packet_, a1, I(count + 1));
        return ZEFC_SEND1(sch, sel_queue, v);
      }
    } else {
      id v = v1;
      (void)ZEFC_IC_SET(self, sel_set_v1, Handler_, v1, ZEFC_IC_GET(v1, sel_link, Packet_, link));
      return ZEFC_SEND1(sch, sel_queue, v);
    }
  }
  return ZEFC_SEND0(sch, sel_suspendCurrent);
}

static id
Packet__addTo_o(id self, int, id queue)
{
  (void)ZEFC_IC_SET(self, sel_set_link, Packet_, link, null_id());
  if (falsy(queue)) {
    return self;
  }
  id next = queue;
  for (;;) {
    id peek = ZEFC_IC_GET(next, sel_link, Packet_, link);
    if (falsy(peek)) {
      break;
    }
    next = peek;
  }
  (void)ZEFC_IC_SET(next, sel_set_link, Packet_, link, self);
  return queue;
}

static void
runRichards()
{
  id scheduler = Scheduler__new();
  (void)ZEFC_SEND4(scheduler, sel_addIdleTask, I(kID_IDLE), I(0), null_id(), I(kCOUNT));

  id queue = Packet__new(null_id(), I(kID_WORKER), I(kKIND_WORK));
  queue = Packet__new(queue, I(kID_WORKER), I(kKIND_WORK));
  (void)ZEFC_SEND3(scheduler, sel_addWorkerTask, I(kID_WORKER), I(1000), queue);

  queue = Packet__new(null_id(), I(kID_DEVICE_A), I(kKIND_DEVICE));
  queue = Packet__new(queue, I(kID_DEVICE_A), I(kKIND_DEVICE));
  queue = Packet__new(queue, I(kID_DEVICE_A), I(kKIND_DEVICE));
  (void)ZEFC_SEND3(scheduler, sel_addHandlerTask, I(kID_HANDLER_A), I(2000), queue);

  queue = Packet__new(null_id(), I(kID_DEVICE_B), I(kKIND_DEVICE));
  queue = Packet__new(queue, I(kID_DEVICE_B), I(kKIND_DEVICE));
  queue = Packet__new(queue, I(kID_DEVICE_B), I(kKIND_DEVICE));
  (void)ZEFC_SEND3(scheduler, sel_addHandlerTask, I(kID_HANDLER_B), I(3000), queue);

  (void)ZEFC_SEND3(scheduler, sel_addDeviceTask, I(kID_DEVICE_A), I(4000), null_id());
  (void)ZEFC_SEND3(scheduler, sel_addDeviceTask, I(kID_DEVICE_B), I(5000), null_id());

  (void)ZEFC_SEND0(scheduler, sel_schedule);

  id qc = ZEFC_IC_GET(scheduler, sel_queueCount, Scheduler_, queueCount);
  id hc = ZEFC_IC_GET(scheduler, sel_holdCount, Scheduler_, holdCount);

  id line1 = String__from_utf8("queue count = ");
  line1 = send(line1, ZEFC_SEL_add_o, ZEFC_SEND0(qc, ZEFC_SEL_toString_o));
  line1 = send(line1, ZEFC_SEL_add_o, String__from_utf8(" (expected "));
  line1 = send(line1, ZEFC_SEL_add_o, ZEFC_SEND0(I(kEXPECTED_QUEUE_COUNT), ZEFC_SEL_toString_o));
  line1 = send(line1, ZEFC_SEL_add_o, String__from_utf8(")"));
  println(line1);

  id line2 = String__from_utf8("hold count = ");
  line2 = send(line2, ZEFC_SEL_add_o, ZEFC_SEND0(hc, ZEFC_SEL_toString_o));
  line2 = send(line2, ZEFC_SEL_add_o, String__from_utf8(" (expected "));
  line2 = send(line2, ZEFC_SEL_add_o, ZEFC_SEND0(I(kEXPECTED_HOLD_COUNT), ZEFC_SEL_toString_o));
  line2 = send(line2, ZEFC_SEL_add_o, String__from_utf8(")"));
  println(line2);

  if (ii(qc) == kEXPECTED_QUEUE_COUNT && ii(hc) == kEXPECTED_HOLD_COUNT) {
    println(String__from_utf8("SUCCESS"));
  } else {
    zefc_error("FAILURE");
  }
}

} // namespace

void
smoke_richards()
{
  ensure_runtime();
  runRichards();
}

} // namespace smoke
} // namespace zefc
