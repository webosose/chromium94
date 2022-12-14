export const description = `
Tests for GPUDevice.lost.
`;

import { Fixture } from '../../../../common/framework/fixture.js';
import { makeTestGroup } from '../../../../common/framework/test_group.js';

export const g = makeTestGroup(Fixture);

g.test('not_lost_on_gc')
  .desc(
    `'lost' is never resolved by GPUDevice being garbage collected (with attemptGarbageCollection).`
  )
  .unimplemented();

g.test('lost_on_destroy')
  .desc(`'lost' is resolved, with reason='destroyed', on GPUDevice.destroy().`)
  .unimplemented();

g.test('same_object')
  .desc(
    `'lost' provides a [new? same?] Promise object with a [new? same?] GPUDeviceLostInfo object
each time it's accessed. (Not sure what the semantic is supposed to be, need to investigate.)`
  )
  .unimplemented();
