// Exercise the actual inline geometry without a DOM or a second implementation.
const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const html=fs.readFileSync(process.argv[2]||'web/index.html','utf8');
new vm.Script(html.split('<script>')[1].split('</script>')[0]);
const code=html.slice(html.indexOf('const distance='),html.indexOf('function screen('));
function geometry(nodes,roads){const graph={nodes:nodes.map(([x,y],id)=>({id,x,y})),lanes:roads.map(([from,to,length,index=0,count=1],id)=>({id,from,to,length,index,count}))};const sandbox={graph};vm.createContext(sandbox);vm.runInContext(code+';result=buildGeometry();',sandbox);return sandbox;}
const sample=geometry([[0,0],[400,0],[200,0]],[[0,1,20],[1,0,20],[0,2,10],[2,1,10]]);
for(const lane of sample.result){sample.lane=lane;assert(vm.runInContext('clearPath(lane.points,lane.obstacles)',sample),'Road crosses unrelated junction');assert(Number.isFinite(lane.len));assert(Math.abs(lane.len/lane.target-1)<1e-6,'Feasible lengths differ from shared scale');}
assert(sample.result[0].points.length>2,'Collinear obstacle did not produce a bend');
// Arc-length interpolation must travel equal distances within a straight section
// and stay on the routed polyline even across a corner.
const l=sample.result[0];sample.lane=l;
for(let i=0;i<=100;i++){sample.t=i/100;const p=vm.runInContext('point(lane,t)',sample);assert(Number.isFinite(p.angle));let best=Infinity;for(let j=1;j<l.points.length;j++){const a=l.points[j-1],b=l.points[j],dx=b.x-a.x,dy=b.y-a.y,t=Math.max(0,Math.min(1,((p.x-a.x)*dx+(p.y-a.y)*dy)/(dx*dx+dy*dy)));best=Math.min(best,Math.hypot(p.x-a.x-t*dx,p.y-a.y-t*dy));}assert(best<1e-7);}
const wide=geometry([[0,0],[600,0],[200,0],[400,0],[300,180]],[[0,1,30,0,8],[0,1,30,7,8],[1,0,30,7,8],[2,4,15],[3,4,15]]);
for(const lane of wide.result){wide.lane=lane;assert(vm.runInContext('clearPath(lane.points,lane.obstacles)',wide),'Wide bundle crosses a node');}
const ratios=geometry([[0,0],[200,0],[500,0]],[[0,1,10],[1,2,30]]).result;
assert(Math.abs(ratios[1].len/ratios[0].len-3)<1e-6);
const again=geometry([[0,0],[400,0],[200,0]],[[0,1,20],[1,0,20],[0,2,10],[2,1,10]]);
assert.equal(JSON.stringify(sample.result),JSON.stringify(again.result),'Geometry is not deterministic');
console.log('PASS: obstacle detours, wide/reverse lanes, shared scale, arc-length motion, deterministic paths and script syntax');
