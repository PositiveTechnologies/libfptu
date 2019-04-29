/*
 * Copyright 2016-2019 libfptu authors: please see AUTHORS file.
 *
 * This file is part of libfptu, aka "Fast Positive Tuples".
 *
 * libfptu is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libfptu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with libfptu.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ***************************************************************************
 *
 * Imported (with simplification) from 1Hippeus project.
 * Copyright (c) 2006-2013 Leonid Yuriev <leo@yuriev.ru>.
 */

#pragma once
#include "fast_positive/details/api.h"

#include "fast_positive/details/warnings_push_system.h"

#include <stddef.h>
#include <stdint.h>

#include "fast_positive/details/warnings_pop.h"

#include "fast_positive/details/warnings_push_pt.h"

typedef union hippeus_actor_C hippeus_actor_t;
union FPTU_API_TYPE hippeus_actor_C {
  struct {
    uint16_t index;
    uint16_t policy;
  };
  int32_t flat;
};

namespace hippeus {

/* LY: Политики-требования участников обмена для hippeus_actor_t и
 * hippeus_thread_assign_policy(). Под участниками прежде всего понимаются
 * треды, но также и обработчики прерываний. Эти значения используются для
 * маркировки участников и определяют обслуживание внутри 1Hippeus. */
enum actor_policy : uint16_t {
  HIPPEUS_NONE =
      0 /*!< Не требуется никаких гарантий, актора можно блокировать до
           освобождения ресурсов. На всех акторов c этой политикой резервируются
           единственный lockfree-слот, который разделяется за счет использования
           мьютекса связанного с ресурсом. Акторам с такой политикой не
           требуется и не назначается уникальный индекс, вместо этого просто
           указывается нуль. Таким образом, взаимодействие с этой политикой
           всегда доступно без какой-либо регистрации и подходит для обращения
           из внешних (незарегистрированных) потоков. */
  ,
  HIPPEUS_BASE =
      1 /*!< Также никаких гарантий, но при назначении этой политики и
           последующих за актором закрепляются персональный lockfree-слот.
             Соответственно поле index в hippeus_actor_tag будет не нулевым. */
  ,
  HIPPEUS_URGENT = 2 /*!< Нельзя выполнять попутную или фоновую работу для
                        общего блага, оставить это другим участникам. */
  ,
  HIPPEUS_NONSTOP = 3 /*!< Нельзя "спать" на мьютексах и других ресурсах, но
                         допустимы TryLock и циклы с CAS, т.е. завершение за
                         недетерминированное кол-во шагов. */
  ,
  HIPPEUS_WAITFREE =
      4 /*!< Самое строгое требование, включает два предыдущих. При занятости
           ресурса или коллизии вместо цикла повтора возвращаяется ошибка.
             Подходит для обработчиков прерываний и жесткого реального времени,
             но при невозможности выполнения (занятости ресурса) будет
           возвращена ошибка. */
  ,
  HIPPEUS_IRQHND =
      5 /*!< Особое требование, аналогично WAITFREE, но дополнительно означает
           что актор может выполняться из обработчика прерывания. Соответственно
           с актором не связан какой-либо поток (thread), у него нет TLS и
           pid/tid. */
};

inline hippeus_actor_t actor_self() {
  hippeus_actor_t actor;
  actor.flat = -1;
  return actor;
}

inline hippeus_actor_t actor_none() {
  hippeus_actor_t actor;
  actor.flat = 0;
  return actor;
}

inline hippeus_actor_t thread_actor() {
  /* just a stub for now */
  return actor_none();
}

} // namespace hippeus

#include "fast_positive/details/warnings_pop.h"
